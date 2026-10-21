#pragma once
#include "IocpEvent.h"
#include "IocpObject.h"
#include "NetAddress.h"
#include "RecvBuffer.h"
#include <functional>

class Service;
class JobQueue;

class Session : public IocpObject
{
	enum
	{
		BUFFER_SIZE = 0x10000, // 64KB
		MAX_SEND_QUEUE_SIZE = 1000,	// send_queue 누적 개수 제한 수.
	};

public:
	Session();
	virtual ~Session();

public:
	void				send(SendBufferRef sendBuffer);
	void				disconnect();
	void				begin_shutdown();
	void				wait_for_shutdown();
	virtual bool		is_closing() const override { return _shutdown_started.load(); }
	// Accept 직후 정원 초과가 확인된 새 세션에서만 호출한다.
	void				reject_connection(int32 max_session_count);
	// 세션마다 작업 큐를 두고, 기존 IOCP 작업 스레드가 큐에 쌓인 작업을 처리한다.
	// 같은 세션의 작업은 동시에 실행되지 않지만, 작업마다 실행 스레드는 달라질 수 있다.
	void				post_logic(std::function<void()> callback);
	void				set_process_connect() { this->process_connect(); }

public:
	void				set_net_address(NetAddress address) { _net_address = address; }
	NetAddress			get_net_address() { return _net_address; }

	SOCKET				get_socket() { return _socket; }

	bool				is_connected() { return _is_connect; }

	SessionRef			get_session() { return static_pointer_cast<Session>(shared_from_this()); }

	RecvBuffer&			get_recv_buffer() { return _recv_buffer; }

	// 계정 ID와 이름은 해당 세션의 작업 큐 안에서만 읽고 변경한다.
	void				set_account_idx(uint64 idx) { _account_idx = idx; }
	uint64				get_account_idx() { return _account_idx; }

	void				set_room_id(int32 room_id) { _enter_room_id.store(room_id); }
	int32				get_room_id() const { return _enter_room_id.load(); }

	uint64				get_last_ping_tick() { return _last_ping_tick; }
	void				set_last_ping_tick(uint64 ping_tick) { _last_ping_tick.exchange(ping_tick); }

	uint64				get_last_pong_tick() { return _last_pong_tick; }
	void				set_last_pong_tick(uint64 pong_tick) { _last_pong_tick.exchange(pong_tick); }

	bool				get_waiting_pong() { return _waiting_pong; }
	void				set_waiting_pong(bool is_waiting) { _waiting_pong.exchange(is_waiting); }

	virtual void		send_ping(uint64 now) { }

	// 이름 조회도 해당 세션의 작업 큐 안에서 수행해야 한다.
	void set_name(wstring name) { _user_name = name; }
	wstring get_name() { return _user_name; }

	void set_service(shared_ptr<Service> sevice) { _service = sevice; }
	weak_ptr<Service> get_service() { return _service; }

private:
	virtual HANDLE		get_handle() override;
	virtual void		dispatch(class IocpEvent* iocpEvent, int32 numOfBytes = 0) override;

private:
	bool				register_disconnect();
	void				register_recv();
	void				register_send();

	void				process_connect();
	void				process_disconnect();
	void				process_recv(int32 numOfBytes);
	void				process_send(int32 numOfBytes);

	void				handle_error(int32 errorCode);
	void				queue_logic(std::function<void()> callback, bool cleanup);

protected:
	// 코어는 프로토콜에 의존하지 않고, 게임 세션이 거절 안내 패킷을 만든다.
	virtual SendBufferRef make_server_full_notice(int32 max_session_count) { return nullptr; }
	virtual void		on_disconnect();
	virtual int32		on_recv(BYTE* get_buffer, int32 len);
	virtual void		on_send(int32 len) { }
	virtual void		on_recv_packet(BYTE* get_buffer, int32 len);

private:
	// 소켓 닫기와 IO 등록을 직렬화한다. 완료 통지는 이 잠금을 잡고 기다리지 않는다.
	mutex _io_lock;
	atomic<bool> _shutdown_started = false;
	atomic<bool> _was_connected = false;

	mutex _logic_lock;
	condition_variable _logic_completed;
	uint32 _pending_jobs = 0;
	bool _logic_stopped = false;
	bool _cleanup_done = false;

	SOCKET				_socket = INVALID_SOCKET;
	NetAddress			_net_address = {};

	atomic<bool>		_is_connect = false;
	atomic<bool>		_is_disconnect = false;
	// 거절 세션은 수신하지 않고 안내 패킷의 송신 완료를 기다린다.
	atomic<bool>		_close_after_send = false;

	weak_ptr<Service> _service;

	// Session과 JobQueue의 shared_from_this가 충돌하지 않도록 상속 대신 멤버로 보관한다.
	shared_ptr<JobQueue> _logic_queue;

protected:

	uint64				_account_idx = 0;

	// 다른 세션의 채팅 전송에서도 방 ID를 조회하므로 atomic으로 보관한다.
	// 방 입퇴장에 따른 값 변경은 여전히 해당 세션의 작업 큐에서만 수행한다.
	atomic<int32>		_enter_room_id = -1;

	wstring				_user_name = L"";

private:
	USE_LOCK;
	RecvBuffer				_recv_buffer;

	queue<SendBufferRef>	_send_queue;
	atomic<bool>			_is_send_register = false;

private:
	ConnectEvent		_connect_event;
	DisconnectEvent		_disconnect_event;
	RecvEvent			_recv_event;
	SendEvent			_send_event;

private:
	atomic<uint64> _last_pong_tick = 0;
	atomic<uint64> _last_ping_tick = 0;
	atomic<bool>   _waiting_pong = false;
};

