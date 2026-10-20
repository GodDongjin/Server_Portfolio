#pragma once
#include "../Utils/Types.h"
#include "../Session/TestSession.h"
#include <thread>

class ClientServer
{
public:
	ClientServer() {}
	~ClientServer() { stop(); }

	bool CreateIocpHandle();
	bool register_socket(shared_ptr<TestSession> session);
	bool start_server(const wstring& ip, uint16 port, SOCKADDR_IN& server_addr);
	bool start_worker_thread(int32 thread_count);
	void stop();

private:
	void dispatch();
	bool dispatch_once(uint32 timeout_ms);

	HANDLE _iocp_handle = nullptr;
	atomic<bool> _stopped = false;
	mutex _lock;
	mutex _stop_lock;
	vector<thread> _worker_threads;
	// IOCP의 raw pointer 키가 가리키는 세션은 마지막 완료 처리까지 살아 있어야 한다.
	vector<shared_ptr<TestSession>> _sessions;
};
