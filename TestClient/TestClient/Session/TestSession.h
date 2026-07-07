#pragma once

#include "../Utils/Types.h"
#include "../Buffer/RecvBuffer.h"
#include "../Buffer/SendBuffer.h"
#include "../NetWork/IocpEvent.h"
#include "../Protocol/Protocol.pb.h"

enum class TEST_SESSION_STATE
{
	NONE,
	DISCONNECTED,
	CONNECTED,
	LOGIN_SEND,
	LOGIN,
	CHAT
};

class TestSession : public enable_shared_from_this<TestSession>
{
public:
	TestSession()
	{
		_recv_buffer = make_shared<RecvBuffer>(0x10000);
		//_send_buffer = make_shared<SendBuffer>(0x10000);
		_test_session_state = TEST_SESSION_STATE::NONE;
	}

	bool start();

	bool connect(SOCKADDR_IN server_addr);
	void disconnect();
	void dispatch(IocpEvent* iocp_evnet, INT32 numOfbyte);

	void send(shared_ptr<SendBuffer> send_buffer);

public:
	void login();
	void test_login(uint32 index, bool is_create_has_account);

	void logout();

	void send_get_room_info();
	void send_enter_room(uint8 room_id);
	void send_exit_room();

	void send_chat(const wstring& message, Protocol::CHAT_STATE chat_state, int32 room_id);
	void send_whisper_chat(const wstring& target_name, const wstring& message, Protocol::CHAT_STATE chat_state, int32 room_id);

public:
	bool is_connected() { return _is_connect; }

	SOCKET* get_socket() { return &_socket; }

	void set_account(uint64 idx) { _account_idx = idx; }
	uint64& get_account() { return _account_idx; }

	void set_test_session_state(TEST_SESSION_STATE state) { _test_session_state = state; }

	void set_bot_index(int32 index) { _bot_index = index; }
	int32 get_bot_index() { return _bot_index; }

	void set_is_login(bool is_login) { _is_login.store(is_login); }
	bool get_is_login() { return _is_login.load(); }

	void set_room_id(int32 room_id) { _room_id = room_id; }
	int32 get_room_id() { return _room_id; }

private:

	bool register_recv();
	bool register_send();

	void process_recv(uint32 num_bytes);
	void process_send(uint32 num_bytes);

private:
	atomic<bool> _is_connect = false;
	atomic<bool> _is_login = false;
	atomic<bool> _is_disconnect = false;

	uint64 _account_idx;
	wstring _name = L"";
	int32 _bot_index = 0;
	int32 _room_id = -1;

	TEST_SESSION_STATE _test_session_state;

private:
	SOCKET _socket = INVALID_SOCKET;

	WSABUF _recv_wsa_buf = {};
	WSABUF _send_wsa_buf = {};
	
	shared_ptr<RecvBuffer> _recv_buffer;

	queue<shared_ptr<SendBuffer>> _send_queue;
	atomic<bool>			_is_send_register = false;

	mutex _send_lock;

private:
	SendEvent _send_event;
	RecvEvent _recv_event;
};




