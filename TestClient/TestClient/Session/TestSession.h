#pragma once

#include "../Utils/Types.h"
#include "../Buffer/RecvBuffer.h"
#include "../Buffer/SendBuffer.h"
#include "../NetWork/IocpEvent.h"
#include "../Protocol/Protocol.pb.h"

class TestSession : public enable_shared_from_this<TestSession>
{
public:
	TestSession() 
	{ 
		_recv_buffer = make_shared<RecvBuffer>(0x10000); 
		//_send_buffer = make_shared<SendBuffer>(0x10000);
	}

	bool start();

	bool connect(SOCKADDR_IN server_addr);
	void disconnect();
	void dispatch(IocpEvent* iocp_evnet, INT32 numOfbyte);

	void send(shared_ptr<SendBuffer> send_buffer);

public:
	void login();
	void logout();
	

	void send_chat(const wstring& message, Protocol::CHAT_STATE chat_state);
public:
	bool is_connected() { return _is_connect; }

	SOCKET* get_socket() { return &_socket; }

	void set_account(uint64 idx) { _account_idx = idx; }
	uint64& get_account() { return _account_idx; }

private:

	bool register_recv();
	bool register_send();

	void process_recv(uint32 num_bytes);
	void process_send(uint32 num_bytes);

private:
	atomic<bool> _is_connect = false;
	bool _is_disconnect = false;

	uint64 _account_idx;

private:
	SOCKET _socket = INVALID_SOCKET;

	/*WSAOVERLAPPED _recv_overlapped = {};
	WSAOVERLAPPED _send_overlapped = {};*/

	WSABUF _recv_wsa_buf = {};
	WSABUF _send_wsa_buf = {};
	
	shared_ptr<RecvBuffer> _recv_buffer;
	//shared_ptr<SendBuffer> _send_buffer;

	queue<shared_ptr<SendBuffer>> _send_queue;
	atomic<bool>			_is_send_register = false;

	mutex _send_lock;

private:
	SendEvent _send_event;
	RecvEvent _recv_event;
};