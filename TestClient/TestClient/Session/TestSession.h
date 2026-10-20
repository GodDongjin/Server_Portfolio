#pragma once

#include "../Utils/Types.h"
#include "../Buffer/RecvBuffer.h"
#include "../Buffer/SendBuffer.h"
#include "../NetWork/IocpEvent.h"
#include "../Protocol/Protocol.pb.h"
#include <condition_variable>
#include <mutex>

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
	TestSession() : _recv_buffer(make_shared<RecvBuffer>(0x10000)) {}
	~TestSession();

	bool start();
	bool connect(SOCKADDR_IN server_addr);
	bool register_iocp(HANDLE iocp_handle);
	void disconnect();
	void dispatch(IocpEvent* iocp_event, INT32 num_bytes);
	bool send(shared_ptr<SendBuffer> send_buffer);

public:
	// 입력은 메인 스레드에서 받고, 세션은 전달받은 값으로 요청만 보낸다.
	bool login(bool is_create, const string& id, const string& pw, const wstring& name);
	bool test_login(uint32 index, bool is_create_has_account);
	void complete_login(bool success, uint64 account_idx = 0);
	bool wait_for_login(uint32 timeout_ms = 10000);
	bool wait_for_disconnect(uint32 timeout_ms = 3000);
	void logout();

	void send_get_room_info();
	bool send_enter_room(uint8 room_id);
	bool send_exit_room();
	void complete_enter_room(bool success, int32 room_id);
	void complete_exit_room(bool success);
	void send_chat(const wstring& message, Protocol::CHAT_STATE chat_state, int32 room_id);
	void send_whisper_chat(const wstring& target_name, const wstring& message,
		Protocol::CHAT_STATE chat_state, int32 room_id);

	// 연결을 닫아도 취소 완료가 남을 수 있다. 호출자는 완료 처리까지 세션을 보관한다.
	void wait_for_io();
	bool has_pending_io();

public:
	bool is_connected() const { return _is_connect.load(); }
	void set_account(uint64 idx) { _account_idx.store(idx); }
	uint64 get_account() const { return _account_idx.load(); }
	void set_test_session_state(TEST_SESSION_STATE state) { _test_session_state.store(state); }
	void set_bot_index(int32 index) { _bot_index = index; }
	int32 get_bot_index() const { return _bot_index; }
	void set_is_login(bool is_login) { _is_login.store(is_login); }
	bool get_is_login() const { return _is_login.load(); }
	int32 get_room_id() const { return _room_id.load(); }

private:
	bool register_recv();
	bool register_send();
	void process_recv(uint32 num_bytes);
	void process_send(uint32 num_bytes);
	bool begin_login();
	void begin_io();
	void finish_io();

	enum class ROOM_REQUEST { NONE, ENTER, EXIT };

	atomic<bool> _is_connect = false;
	atomic<bool> _is_login = false;
	atomic<bool> _is_disconnect = false;
	atomic<bool> _recv_started = false;
	atomic<uint64> _account_idx = 0;
	atomic<int32> _room_id = -1;
	atomic<TEST_SESSION_STATE> _test_session_state = TEST_SESSION_STATE::NONE;
	int32 _bot_index = 0;

	// 응답 대기 상태는 요청 시작·완료·연결 종료에서 같은 잠금으로 변경한다.
	bool _login_pending = false;
	ROOM_REQUEST _room_request = ROOM_REQUEST::NONE;
	condition_variable _state_changed;
	SOCKET _socket = INVALID_SOCKET;
	shared_ptr<RecvBuffer> _recv_buffer;
	queue<shared_ptr<SendBuffer>> _send_queue;
	atomic<bool> _is_send_register = false;

	// 송신 큐·이벤트와 소켓 등록·종료를 함께 보호한다.
	mutex _send_lock;
	SendEvent _send_event;
	RecvEvent _recv_event;

	// 완료 콜백 실행까지 포함해서 세므로 작업 스레드를 너무 일찍 종료하지 않는다.
	mutex _io_lock;
	condition_variable _io_completed;
	int32 _pending_io = 0;
};
