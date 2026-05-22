#pragma once
#include "../Utils/Types.h"

class ClientServer
{
public:

	bool CreateIocpHandle();
	bool register_socket(SOCKET* client_socket);

	SOCKADDR_IN start_server(const wstring& ip, uint16 port);
	void start_worker_thread(short thread_count);

	void dispatch();

private:
	HANDLE _iocp_handle;
	SOCKADDR_IN server_addr;

private:
	mutex _lock;
	vector<thread> _worker_trheads;
};

