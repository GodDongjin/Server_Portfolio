#include "pch.h"
#include "Listener.h"
#include "Service.h"
#include "Session.h"
#include "SocketUtils.h"
#include "JobTimer.h"
#include "ThreadManager.h"

namespace
{
	// 현재 Listener는 최대 접속 수와 별개로 32개 Accept 슬롯을 유지한다.
	constexpr int32 accept_slots = 32;
	uint64 first_accept_tick = 0;
	int32 accept_call_count = 0;
	int32 failures_left = 0;
	int32 accept_error = WSAENOBUFS;
	bool complete_immediately = false;
	uint64 last_accept_tick = 0;

	void require_accept(bool condition, const char* message)
	{
		if (!condition)
		{
			cerr << "FAIL: " << message << endl;
			abort();
		}
	}

	// 테스트에서는 실제 비동기 Accept를 등록하지 않고 반환 결과만 주입한다.
	BOOL PASCAL test_accept(SOCKET, SOCKET, PVOID, DWORD, DWORD, DWORD,
		LPDWORD bytesReceived, LPOVERLAPPED)
	{
		++accept_call_count;
		last_accept_tick = ::GetTickCount64();
		if (accept_call_count == 1)
			first_accept_tick = last_accept_tick;
		*bytesReceived = 0;
		if (failures_left > 0)
		{
			--failures_left;
			::WSASetLastError(accept_error);
			return FALSE;
		}

		if (complete_immediately) {
			return TRUE;
		}

		::WSASetLastError(WSA_IO_PENDING);
		return FALSE;
	}

	void set_accept_result(int32 errorCode, int32 failureCount)
	{
		accept_call_count = 0;
		failures_left = failureCount;
		accept_error = errorCode;
		complete_immediately = false;
		last_accept_tick = 0;
	}

	void distribute_at(uint64 tick)
	{
		LEndTickCount = ::GetTickCount64() + 1000;
		GJobTimer->distribute(tick);
		ThreadManager::do_global_queue_work();
	}

	void distribute_due_jobs()
	{
		// 테스트에서만 타이머 조회 시각을 앞당겨 실제 대기 없이 예약 작업을 실행한다.
		distribute_at(::GetTickCount64() + 1000);
	}

	class TestListener : public Listener
	{
	public:
		AcceptEvent* get_test_event()
		{
			return _accept_events.at(0);
		}

		void release_test_events()
		{
			// test_accept는 커널 I/O를 등록하지 않으므로 테스트가 완료 참조를 직접 정리한다.
			for (AcceptEvent* acceptEvent : _accept_events)
			{
				if (acceptEvent->get_owner())
					finish_io();
				acceptEvent->set_owner(nullptr);
				acceptEvent->set_session(nullptr);
			}
		}
	};

	struct AcceptFixture
	{
		explicit AcceptFixture(SessionFactory factory = []() { return make_shared<Session>(); })
		{
			service = make_shared<ServerService>(
				NetAddress(L"127.0.0.1", 0), make_shared<IocpCore>(),
				factory, make_shared<SessionManager>(1));
			listener = make_shared<TestListener>();
		}

		~AcceptFixture()
		{
			if (listener != nullptr)
			{
				listener->close_socket();
				listener->release_test_events();
			}
			distribute_due_jobs();
		}

		void start()
		{
			require_accept(listener->start_accept(service), "test listener starts");
		}

		ServerServiceRef service;
		shared_ptr<TestListener> listener;
	};

	class AcceptTestScope
	{
	public:
		AcceptTestScope()
		{
			_previous_accept = SocketUtils::AcceptEx;
			SocketUtils::AcceptEx = test_accept;
			_previous_directory = filesystem::current_path();
			_directory = filesystem::temp_directory_path() /
				("accept-retry-" + to_string(::GetCurrentProcessId()) + "-" +
				 to_string(::GetTickCount64()));
			require_accept(filesystem::create_directory(_directory), "create isolated log directory");
			filesystem::current_path(_directory);
			GLogger->init("Log");
		}

		~AcceptTestScope()
		{
			SocketUtils::AcceptEx = _previous_accept;
			GLogger->shutdown();
			filesystem::current_path(_previous_directory);
			// 이 테스트가 새로 만든 임시 디렉터리만 정리한다.
			filesystem::remove_all(_directory);
		}

	private:
		LPFN_ACCEPTEX _previous_accept = nullptr;
		filesystem::path _previous_directory;
		filesystem::path _directory;
	};
}

void test_accept_retry()
{
	AcceptTestScope scope;

	for (int32 errorCode : { WSAENOBUFS, WSAEMFILE, WSAECONNRESET, WSAECONNABORTED })
	{
		set_accept_result(errorCode, accept_slots * 4);
		AcceptFixture fixture;
		fixture.start();
		require_accept(accept_call_count == accept_slots, "immediate failure must not recurse");
		auto acceptEvent = fixture.listener->get_test_event();
		require_accept(acceptEvent->get_owner() == nullptr && acceptEvent->get_session() == nullptr,
			"failed Accept releases listener and session references");

		// 등록 실패 시각에서 99ms까지는 재시도하지 않아야 한다.
		distribute_at(first_accept_tick + 99);
		require_accept(accept_call_count == accept_slots, "retry must wait at least 100ms");
		for (int32 expectedCount = 2; expectedCount <= 5; ++expectedCount)
		{
			distribute_due_jobs();
			require_accept(accept_call_count == accept_slots * expectedCount, "one attempt per timer callback");
		}

		// 마지막 시도는 WSA_IO_PENDING이므로 추가 예약이 없어야 한다.
		distribute_due_jobs();
		require_accept(accept_call_count == accept_slots * 5, "pending Accept must not be retried");
		require_accept(acceptEvent->get_owner() == fixture.listener && acceptEvent->get_session() != nullptr,
			"pending Accept retains listener and session");
	}
	cout << "PASS: Accept retries after 100ms; repeated failures do not recurse; pending keeps ownership" << endl;

	for (int32 errorCode : { WSAEINVAL, WSAENOTSOCK, WSANOTINITIALISED })
	{
		set_accept_result(errorCode, accept_slots);
		AcceptFixture fixture;
		fixture.start();
		distribute_due_jobs();
		require_accept(accept_call_count == accept_slots, "non-retryable error stops this Accept slot");
		require_accept(fixture.listener->get_test_event()->get_owner() == nullptr,
			"terminal error releases event owner");
	}

	{
		set_accept_result(0, 0);
		complete_immediately = true;
		AcceptFixture fixture;
		fixture.start();
		distribute_due_jobs();
		require_accept(accept_call_count == accept_slots, "synchronous success waits for completion without retry");
	}
	cout << "PASS: permanent errors stop retry; synchronous success is not duplicated" << endl;

	{
		set_accept_result(0, 0);
		int32 factory_call_count = 0;
		AcceptFixture fixture([&]() -> SessionRef
		{
			if (++factory_call_count <= accept_slots) {
				return nullptr;
			}
			return make_shared<Session>();
		});
		fixture.start();
		require_accept(factory_call_count == accept_slots && accept_call_count == 0,
			"null session is rejected before AcceptEx");
		distribute_due_jobs();
		require_accept(factory_call_count == accept_slots * 2 && accept_call_count == accept_slots,
			"session creation recovers through the timer");
	}
	cout << "PASS: session creation failure retries without null dereference" << endl;

	{
		set_accept_result(WSAENOBUFS, accept_slots * 2);
		AcceptFixture fixture;
		fixture.start();
		fixture.listener->close_socket();
		fixture.listener->close_socket();
		distribute_due_jobs();
		distribute_due_jobs();
		require_accept(accept_call_count == accept_slots, "queued retry cannot register after close");
	}

	{
		set_accept_result(WSAENOBUFS, accept_slots * 2);
		AcceptFixture fixture;
		fixture.start();
		weak_ptr<TestListener> weak_listener = fixture.listener;
		fixture.listener.reset();
		require_accept(weak_listener.expired(), "queued retry does not retain a destroyed listener");
		distribute_due_jobs();
		require_accept(accept_call_count == accept_slots, "expired listener cancels queued retry");
	}
	cout << "PASS: listener close and destruction cancel delayed Accept registration" << endl;
}
