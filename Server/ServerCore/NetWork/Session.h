#pragma once
#include "IocpEvent.h"
#include "IocpObject.h"
#include "NetAddress.h"
#include "RecvBuffer.h"

class Service;

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
	void				set_process_connect() { this->process_connect(); }

public:
	void				set_net_address(NetAddress address) { _net_address = address; }
	NetAddress			get_net_address() { return _net_address; }

	SOCKET				get_socket() { return _socket; }

	bool				is_connected() { return _is_connect; }

	SessionRef			get_session() { return static_pointer_cast<Session>(shared_from_this()); }

	RecvBuffer&			get_recv_buffer() { return _recv_buffer; }

	void				set_account_idx(uint64 idx) { _account_idx = idx; }
	uint64				get_account_idx() { return _account_idx; }

	void				set_room_id(int32 room_id) { _enter_room_id = room_id; }
	int32				get_room_id() { return _enter_room_id; }

	uint64				get_last_ping_tick() { return _last_ping_tick; }
	void				set_last_ping_tick(uint64 ping_tick) { _last_ping_tick.exchange(ping_tick); }

	uint64				get_last_pong_tick() { return _last_pong_tick; }
	void				set_last_pong_tick(uint64 pong_tick) { _last_pong_tick.exchange(pong_tick); }

	bool				get_waiting_pong() { return _waiting_pong; }
	void				set_waiting_pong(bool is_waiting) { _waiting_pong.exchange(is_waiting); }

	virtual void		send_ping(uint64 now) { }

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

protected:
	virtual void		on_disconnect();
	virtual int32		on_recv(BYTE* get_buffer, int32 len);
	virtual void		on_send(int32 len) { }
	virtual void		on_recv_packet(BYTE* get_buffer, int32 len);

private:
	SOCKET				_socket = INVALID_SOCKET;
	NetAddress			_net_address = {};

	atomic<bool>		_is_connect = false;
	atomic<bool>		_is_disconnect = false;

	weak_ptr<Service> _service;

protected:
	uint64				_account_idx = 0;
	int32				_enter_room_id = -1;
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

