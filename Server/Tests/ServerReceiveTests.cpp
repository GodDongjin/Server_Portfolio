#include "pch.h"
#include "Session.h"
#include "Service.h"
#include "SocketUtils.h"
#include "ThreadManager.h"

int WSAAPI test_server_recv(SOCKET, LPWSABUF, DWORD, LPDWORD, LPDWORD,
	LPWSAOVERLAPPED, LPWSAOVERLAPPED_COMPLETION_ROUTINE);

// 제품의 수신 등록 경로를 그대로 실행하고 이 테스트에서만 OS 반환값을 주입한다.
#define WSARecv test_server_recv
#include "../ServerCore/NetWork/Session.cpp"
#undef WSARecv

namespace
{
	int32 recv_error = WSA_IO_PENDING;
	int32 recv_calls = 0;
	int32 disconnect_calls = 0;
	IocpEvent* recv_event = nullptr;

	void require_recv(bool condition, const char* message)
	{
		if (!condition)
		{
			cerr << "FAIL: " << message << endl;
			abort();
		}
	}

	BOOL PASCAL test_disconnect(SOCKET, LPOVERLAPPED, DWORD, DWORD)
	{
		++disconnect_calls;
		// 실제 연결 없이 만든 테스트 소켓이므로 기존 동기 종료 정리 경로로 보낸다.
		::WSASetLastError(WSAENOTCONN);
		return FALSE;
	}

	class ReceiveSession : public Session
	{
	public:
		explicit ReceiveSession(int32& cleanup_count) : _cleanup_count(cleanup_count) {}

	protected:
		void on_disconnect() override
		{
			++_cleanup_count;
		}

	private:
		int32& _cleanup_count;
	};

	void drain_jobs()
	{
		LEndTickCount = ::GetTickCount64() + 1000;
		ThreadManager::do_global_queue_work();
	}

	void test_receive_result(int32 errorCode)
	{
		recv_error = errorCode;
		recv_calls = 0;
		disconnect_calls = 0;
		recv_event = nullptr;
		int32 cleanup_count = 0;
		auto manager = make_shared<SessionManager>(1);
		auto service = make_shared<ServerService>(
			NetAddress(L"127.0.0.1", 0), make_shared<IocpCore>(),
			[]() { return make_shared<Session>(); }, manager);
		auto session = make_shared<ReceiveSession>(cleanup_count);
		session->set_service(service);
		session->set_account_idx(1);
		session->set_name(L"receive-test");
		manager->add_account_session(1, session);
		manager->add_name_session(L"receive-test", session);
		weak_ptr<Session> weak_session = session;

		require_recv(manager->add_session(session), "session is admitted before receive registration");
		require_recv(recv_calls == 1 && recv_event != nullptr, "receive registered exactly once");
		if (errorCode == WSA_IO_PENDING || errorCode == 0)
		{
			require_recv(session->is_connected() && disconnect_calls == 0,
				"pending and synchronous success stay connected until completion");
			require_recv(recv_event->get_owner() == session, "in-flight receive retains its owner");
			// 가짜 수신은 커널에 등록되지 않았으므로 테스트가 EOF 완료를 전달한다.
			auto owner = recv_event->get_owner();
			owner->dispatch(recv_event, 0);
		}

		require_recv(!session->is_connected(), "receive failure disconnects immediately");
		require_recv(recv_event->get_owner() == nullptr, "receive owner released without completion");
		require_recv(disconnect_calls == 1 && cleanup_count == 0,
			"disconnect runs once and logic cleanup stays queued");
		session->disconnect();
		drain_jobs();
		require_recv(cleanup_count == 1 && disconnect_calls == 1, "cleanup is performed exactly once");
		require_recv(!manager->is_whisper_target_find(L"receive-test"), "name registration released");
		require_recv(session->get_account_idx() == 0 && session->get_name().empty(),
			"account state cleared after queued cleanup");
		session.reset();
		require_recv(weak_session.expired(), "no session or I/O owner reference leaks");

		auto replacement = make_shared<ReceiveSession>(cleanup_count);
		replacement->set_service(service);
		recv_error = WSA_IO_PENDING;
		require_recv(manager->add_session(replacement), "failed session releases capacity");
		{
			auto owner = recv_event->get_owner();
			owner->dispatch(recv_event, 0);
		}
		drain_jobs();
	}
}

int WSAAPI test_server_recv(SOCKET, LPWSABUF, DWORD, LPDWORD, LPDWORD,
	LPWSAOVERLAPPED overlapped, LPWSAOVERLAPPED_COMPLETION_ROUTINE)
{
	++recv_calls;
	recv_event = static_cast<IocpEvent*>(overlapped);
	if (recv_error == 0) {
		return 0;
	}
	::WSASetLastError(recv_error);
	return SOCKET_ERROR;
}

void test_server_receive()
{
	const auto previous_disconnect = SocketUtils::DisconnectEx;
	SocketUtils::DisconnectEx = test_disconnect;
	const auto previous_directory = filesystem::current_path();
	const auto directory = filesystem::temp_directory_path() /
		("server-receive-" + to_string(::GetCurrentProcessId()) + "-" + to_string(::GetTickCount64()));
	require_recv(filesystem::create_directory(directory), "create isolated receive test directory");
	filesystem::current_path(directory);
	GLogger->init("Log");

	const int32 receive_results[] = { WSAENOBUFS, WSAENETDOWN, WSAENOTSOCK, WSAEINVAL,
		WSAECONNRESET, WSAECONNABORTED, WSAESHUTDOWN, WSA_IO_PENDING, 0 };
	for (int32 errorCode : receive_results) {
		test_receive_result(errorCode);
	}

	SocketUtils::DisconnectEx = previous_disconnect;
	filesystem::current_path(previous_directory);
	// 이번 테스트가 직접 만든 고유 임시 디렉터리만 정리한다.
	require_recv(filesystem::canonical(directory).parent_path() ==
		filesystem::canonical(filesystem::temp_directory_path()),
		"cleanup stays inside the temporary directory");
	filesystem::remove_all(directory);
	cout << "PASS: receive errors release session, account, event owner and capacity; pending/success preserved" << endl;
}
