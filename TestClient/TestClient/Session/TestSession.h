#pragma once

#include "../Utils/Types.h"
#include "../Buffer/RecvBuffer.h"
#include "../Buffer/SendBuffer.h"
#include "../NetWork/IocpEvent.h"

class TestSession
{
public:
	TestSession() 
	{ 
		_recv_buffer = make_shared<RecvBuffer>(0x10000); 
		_send_buffer = make_shared<SendBuffer>(0x10000);
	}

	bool connect(SOCKADDR_IN server_addr);
	void disconnect();

	void send(/*shared_ptr<SendBuffer> send_buffer*/);

	void dispatch(IocpEvent* iocp_evnet);

public:
	bool is_connected() { return _is_connect; }

private:

	bool register_recv();
	bool register_send();

	void process_recv(uint32 num_bytes);
	void process_send(uint32 num_bytes);

	//void MakeSendBuffer()

private:
	atomic<bool>		_is_connect = false;

private:
	SOCKET _socket = INVALID_SOCKET;

	WSAOVERLAPPED _recv_overlapped = {};
	WSAOVERLAPPED _send_overlapped = {};

	WSABUF _recv_wsa_buf = {};
	WSABUF _send_wsa_buf = {};
	
	shared_ptr<RecvBuffer> _recv_buffer;
	shared_ptr<SendBuffer> _send_buffer;
};