#include "pch.h"
#include "../ServerCore/NetWork/Service.h"
#include "../ServerCore/NetWork/IocpCore.h"
#include "../ServerCore/DB/DBManager.h"
#include "../ServerCore/Thread/ThreadManager.h"
#include "../ServerCore/Utils/Logger.h"
#include "../ServerCore/Utils/ConfigReader.h"
#include <condition_variable>
#include <chrono>
#include <mutex>
#include "../ServerCore/Job/JobTimer.h"

#include "GameSession.h"
#include "../Packet/ServerPacketHandler.h"
#include "../Utils/GameGlobal.h"

class StopSignal
{
private:
	mutex _lock;
	condition_variable _changed;
	bool _requested = false;

public:
	// 종료 요청이 오면 주기 작업의 남은 대기 시간을 즉시 끝낸다.
	bool wait(uint32 interval_ms)
	{
		unique_lock<mutex> guard(_lock);
		return _changed.wait_for(guard, chrono::milliseconds(interval_ms),
			[this]() { return _requested; });
	}

	void request()
	{
		{
			lock_guard<mutex> guard(_lock);
			_requested = true;
		}
		_changed.notify_all();
	}
};

void do_worker_job(ServerServiceRef service, atomic<bool>& workers_running)
{
	while (workers_running.load())
	{
		LEndTickCount = ::GetTickCount64() + 64;

		// 스레드 네트워크 IO, packet 로직 처리
		service->get_iocp_core()->dispatch(10);

		// 스레드에 작업 분배 등록
		ThreadManager::distribute_reserved_jobs();

		// 스레드 분배된 작업 처리
		ThreadManager::do_global_queue_work();

	}
}


void print_server_stats(uint64& prevRecvChat, uint64& prevBroadcastTarget)
{
	uint64 recvChat = GServerStats.recv_chat.load();
	uint64 broadcastTarget = GServerStats.broadcast_target.load();

	uint64 recvChatDelta = recvChat - prevRecvChat;
	uint64 broadcastDelta = broadcastTarget - prevBroadcastTarget;

	prevRecvChat = recvChat;
	prevBroadcastTarget = broadcastTarget;

	wcout << L"[SERVER] "
		<< L"connect = " << GServerStats.connect.load()
		<< L" login = " << GServerStats.login.load()
		<< L" recv_chat = " << recvChat
		<< L" recv_chat/s = " << recvChatDelta
		<< L" broadcast_target = " << broadcastTarget
		<< L" broadcast/s = " << broadcastDelta
		<< L" send_complete = " << GServerStats.send_complete.load()
		<< L" send_bytes = " << GServerStats.send_complete_bytes.load()
		<< L" disconnect = " << GServerStats.disconnect.load()
		<< endl;
}

int main()
{
	std::locale::global(std::locale(""));
	wcin.imbue(std::locale());
	wcout.imbue(std::locale());

	ServerPacketHandler::Init();
	ConfigReader config(L"Config/config.ini");
	if (!config.load())
	{
		wcerr << L"Failed to load config.ini" << endl;
		return 1;
	}

	const wstring server_ip = config.get_wstring("server", "ip");
	const uint16 port_value = static_cast<uint16>(config.get_int("server", "port"));
	const int32 session_max_count = config.get_int("server", "session_max_count");
	const int32 worker_thread_count = config.get_int("server", "work_thread_count");

	if (port_value <= 0 || port_value > 65535)
	{
		wcerr << L"Invalid server port: " << port_value << endl;
		return 0;
	}

	if (session_max_count <= 0)
	{
		wcerr << L"Invalid session max count: " << session_max_count << endl;
		return 0;
	}

	if (worker_thread_count <= 0)
	{
		wcerr << L"Invalid worker thread count: " << worker_thread_count << endl;
		return 0;
	}


	GLogger->init("./Log");

	ServerServiceRef service = make_shared<ServerService>(
		NetAddress(server_ip, port_value),
		make_shared<IocpCore>(),
		[=]() {return make_shared<GameSession>(); },
		make_shared<SessionManager>(session_max_count));

	if (!service->start())
	{
		wcerr << L"Failed to start server" << endl;
		return 1;
	}

	// Enable this when the DB system is configured.
	/*ASSERT_CRASH(GDBManager->db_connect_start(
		config.get_wstring("database_info", "driver"),
		config.get_wstring("database_info", "ip"),
		config.get_wstring("database_info", "database"),
		config.get_wstring("database_info", "user"),
		config.get_wstring("database_info", "password"))
	);*/

	atomic<bool> workers_running = true;
	StopSignal background_stop;
	ThreadManager background_threads;

	for (int32 i = 0; i < worker_thread_count; i++)
	{
		GThreadManager->launch([service, &workers_running]()
		{
			do_worker_job(service, workers_running);
		});
	}

	background_threads.launch([service, &background_stop]()
	{
		do
		{
			service->get_sessionManager()->check_heartbeat(::GetTickCount64());
		}
		while (!background_stop.wait(5000));
	});
	background_threads.launch([&background_stop]()
	{
		uint64 prevRecvChat = 0;
		uint64 prevBroadcastTarget = 0;
		do
		{
			print_server_stats(prevRecvChat, prevBroadcastTarget);
		}
		while (!background_stop.wait(1000));
	});

	wcout << L"서버를 종료하려면 /q를 입력하세요." << endl;
	wstring command;
	while (getline(wcin, command))
	{
		if (command == L"/q")
			break;
	}

	// 주기 작업을 먼저 멈추되, 연결 정리에 필요한 IOCP 작업 스레드는 유지한다.
	background_stop.request();
	background_threads.join();
	service->close_service();

	workers_running.store(false);
	GThreadManager->join();
	GJobTimer->clear();
	service.reset();

	// 모든 연결 정리가 끝난 뒤 최종 누적 통계를 콘솔에 표시한다.
	uint64 final_recv_chat = GServerStats.recv_chat.load();
	uint64 final_broadcast_target = GServerStats.broadcast_target.load();
	print_server_stats(final_recv_chat, final_broadcast_target);
	wcout << L"서버 정상 종료 완료" << endl;
	return 0;
}
