#pragma once

#include "../Utils/Types.h"
#include "../Buffer/RecvBuffer.h"
#include "../Buffer/SendBuffer.h"

class TestSession
{
public:
	TestSession() 
	{ 
		_recv_buffer = make_shared<RecvBuffer>(0x10000); 
		_send_buffer = make_shared<SendBuffer>(0x10000);
	}

	bool connect(const wstring& ip, uint16 port);
	void disconnect();

	bool register_recv();
	bool send_packet(const vector<BYTE>& packet);

	void process_recv(uint32 num_bytes);
	void process_send(uint32 num_bytes);

	//void MakeSendBuffer()

private:
	SOCKET _socket;

	WSAOVERLAPPED _recv_overlapped = {};
	WSAOVERLAPPED _send_overlapped = {};

	WSABUF _recv_wsa_buf = {};
	WSABUF _send_wsa_buf = {};
	
	shared_ptr<RecvBuffer> _recv_buffer;
	shared_ptr<SendBuffer> _send_buffer;
};