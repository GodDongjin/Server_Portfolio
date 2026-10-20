#include "pch.h"
#include "Session.h"
#include "SendBuffer.h"
#include "SessionManager.h"
#include "GlobalQueue.h"
#include "JobQueue.h"
#include "ThreadManager.h"
#include "LockQueue.h"
#include <condition_variable>
#include <thread>

static void require(bool ok, const char* message)
{
    if (!ok)
    {
        std::cerr << "FAIL: " << message << std::endl;
        std::abort();
    }
}

static void test_partial_send()
{
    // 여러 버퍼에 걸친 부분 송신을 가정해 실제 완료 바이트만 소비하는지 검사한다.
    auto first = make_shared<SendBuffer>(5);
    auto second = make_shared<SendBuffer>(4);
    auto third = make_shared<SendBuffer>(3);
    first->close(5);
    second->close(4);
    third->close(3);
    SendEvent event;
    event.send_Buffers_push(first);
    event.send_Buffers_push(second);
    event.send_Buffers_push(third);
    vector<WSABUF> buffers;
    event.build_wsabufs(buffers);
    require(buffers.size() == 3 && buffers[0].len == 5, "initial send contains all buffers");
    require(!event.consume_sent_bytes(0) && !event.consume_sent_bytes(-1)
            && !event.consume_sent_bytes(13), "invalid completion is rejected without consuming");
    require(event.consume_sent_bytes(2), "partial first buffer");
    event.build_wsabufs(buffers);
    require(buffers.size() == 3 && buffers[0].len == 3
            && buffers[0].buf == reinterpret_cast<char*>(first->get_buffer()) + 2,
            "resend begins after completed bytes");
    require(event.consume_sent_bytes(6), "completion spanning two buffers");
    event.build_wsabufs(buffers);
    require(buffers.size() == 2 && buffers[0].len == 1
            && buffers[0].buf == reinterpret_cast<char*>(second->get_buffer()) + 3,
            "cross-buffer completion retains correct offset");
    require(first->get_write_size() == 5 && second->get_write_size() == 4,
            "shared broadcast buffers are not modified");
    require(event.consume_sent_bytes(1), "completion exactly on buffer boundary");
    event.build_wsabufs(buffers);
    require(buffers.size() == 1 && buffers[0].len == 3 && event.has_pending_data(),
            "last buffer is still pending");
    require(event.consume_sent_bytes(3) && !event.has_pending_data(),
            "close is allowed only after the final byte");
    event.build_wsabufs(buffers);
    require(buffers.empty() && !event.consume_sent_bytes(1), "completed batch cannot consume more");
    std::weak_ptr<SendBuffer> weakFirst = first;
    first.reset();
    require(!weakFirst.expired(), "event retains buffers until completion cleanup");
    event.send_Buffers_clear();
    require(weakFirst.expired(), "completion cleanup releases buffers");
    event.send_Buffers_push(third);
    event.build_wsabufs(buffers);
    require(buffers.size() == 1 && buffers[0].len == 3, "new batch starts from offset zero");
    require(event.consume_sent_bytes(3) && !event.has_pending_data(), "new batch completes");
    std::cout << "PASS: partial sends, buffer boundaries, completion validation and lifetime" << std::endl;
}

void test_accept_retry();
void test_server_receive();

int main()
{
    test_accept_retry();
    test_server_receive();
    test_partial_send();
    // pop_all이 중복 잠금으로 멈추지 않고, 값이 0인 항목도 빠짐없이 꺼내는지 확인한다.
    LockQueue<int> values;
    values.push(0);
    values.push(1);
    vector<int> batch;
    values.pop_all(batch);
    require(batch == vector<int>({0, 1}), "pop_all drains in FIFO order");
    batch.clear();
    values.pop_all(batch);
    require(batch.empty(), "pop_all on empty queue");

    constexpr int sessionCount = 8, producerCount = 4, jobsPerProducer = 1000;
    vector<SessionRef> sessions;
    for (int i = 0; i < sessionCount; ++i)
        sessions.push_back(make_shared<Session>());

    // 실제 IOCP 작업 스레드와 동일한 함수로 전역 작업 큐를 처리한다.
    atomic<bool> stopping = false;
    vector<thread> workers;
    for (int i = 0; i < 4; ++i)
        workers.emplace_back([&]()
        {
            while (!stopping.load())
            {
                LEndTickCount = GetTickCount64() + 2;
                ThreadManager::do_global_queue_work();
                std::this_thread::yield();
            }
        });

    atomic<int> active[sessionCount]{};
    int sequence[sessionCount][producerCount]{};
    atomic<int> completed = 0;
    vector<thread> producers;
    for (int p = 0; p < producerCount; ++p)
        producers.emplace_back([&, p]()
        {
            for (int i = 0; i < jobsPerProducer; ++i)
                for (int s = 0; s < sessionCount; ++s)
                    sessions[s]->post_logic([&, p, i, s]()
                    {
                        require(active[s].fetch_add(1) == 0, "session jobs must not overlap");
                        require(sequence[s][p]++ == i, "FIFO order per producer");
                        if (i % 17 == 0)
                            std::this_thread::yield();
                        active[s].fetch_sub(1);
                        completed.fetch_add(1);
                    });
        });
    for (auto& producer : producers)
        producer.join();

    const auto deadline = GetTickCount64() + 10000;
    while (completed.load() != sessionCount * producerCount * jobsPerProducer)
    {
        require(GetTickCount64() < deadline, "all queued jobs complete");
        Sleep(1);
    }

    // 한 세션의 작업이 대기해도 다른 세션은 진행되고, 작업 안에서 등록한 작업은 나중에 실행되어야 한다.
    std::mutex gateMutex;
    std::condition_variable gate;
    bool firstStarted = false, secondCompleted = false;
    atomic<bool> nestedCompleted = false;
    sessions[0]->post_logic([&]()
    {
        std::unique_lock<std::mutex> lock(gateMutex);
        firstStarted = true;
        gate.notify_all();
        require(gate.wait_for(lock, std::chrono::seconds(5), [&]() { return secondCompleted; }),
                "another session progresses while this one is blocked");
        sessions[0]->post_logic([&]() { nestedCompleted.store(true); });
        require(!nestedCompleted.load(), "post_logic must not execute nested work inline");
    });
    {
        std::unique_lock<std::mutex> lock(gateMutex);
        require(gate.wait_for(lock, std::chrono::seconds(5), [&]() { return firstStarted; }),
                "first session starts");
    }
    sessions[1]->post_logic([&]()
    {
        std::lock_guard<std::mutex> lock(gateMutex);
        secondCompleted = true;
        gate.notify_all();
    });
    const auto nestedDeadline = GetTickCount64() + 5000;
    while (!nestedCompleted.load())
    {
        require(GetTickCount64() < nestedDeadline, "nested work completes");
        Sleep(1);
    }
    stopping.store(true);
    for (auto& worker : workers)
        worker.join();

    // 같은 이름이 새 세션에 등록된 뒤 이전 세션의 종료 정리가 실행되는 상황을 확인한다.
    SessionManager manager(10);
    manager.add_name_session(L"reused-name", sessions[0]);
    manager.add_name_session(L"reused-name", sessions[1]);
    manager.release_name_session(L"reused-name", sessions[0]);
    require(manager.is_whisper_target_find(L"reused-name"), "stale cleanup keeps new owner");
    manager.release_name_session(L"reused-name", sessions[1]);
    require(!manager.is_whisper_target_find(L"reused-name"), "current owner can remove entry");

    // 호출자가 참조를 해제해도 큐 작업의 shared_ptr 캡처가 세션 수명을 유지하는지 확인한다.
    auto owner = make_shared<Session>();
    std::weak_ptr<Session> weakOwner = owner;
    bool ran = false;
    owner->post_logic([self = owner, &ran]() { ran = true; });
    owner.reset();
    require(!weakOwner.expired(), "queued callback retains session");
    LEndTickCount = GetTickCount64() + 1000;
    ThreadManager::do_global_queue_work();
    require(ran && weakOwner.expired(), "session released after last callback");

    std::cout << "PASS: 32000 jobs / 4 producers / 4 workers; FIFO, serialization, "
                 "independent sessions, nested enqueue, ownership and lifetime" << std::endl;
}
