#pragma once
#include "../Utils/Types.h"
#include "../Session/TestSession.h"

class ClientServer
{
public:
	ClientServer() {};
	~ClientServer() { stop(); };


	bool CreateIocpHandle();
	bool register_socket(shared_ptr<TestSession> session);

	SOCKADDR_IN start_server(const wstring& ip, uint16 port);
	void start_worker_thread(short thread_count);

	void stop();

private:
	void dispatch();

private:
	HANDLE _iocp_handle = nullptr;
	atomic<bool> _stopped = false;

	SOCKADDR_IN server_addr;

private:
	mutex _lock;
	vector<thread> _worker_threads;
};

