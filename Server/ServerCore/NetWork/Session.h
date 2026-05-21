#pragma once
#include "IocpEvent.h"
#include "Service.h"
#include "IocpObject.h"
#include "NetAddress.h"
#include "RecvBuffer.h"

struct PacketHeader
{
	uint16 size;
	uint16 id;
};

class Session : public IocpObject
{
	enum
	{
		BUFFER_SIZE = 0x10000, // 64KB
	};

public:
	Session();
	virtual ~Session();

public:
	void				send(SendBufferRef sendBuffer);
	void				disconnect(const WCHAR* cause);
	void				set_process_connect() { this->process_connect(); }

public:
	void				set_net_address(NetAddress address) { _net_address = address; }
	NetAddress			get_net_address() { return _net_address; }

	SOCKET				get_socket() { return _socket; }

	bool				is_connected() { return _is_connect; }

	SessionRef			get_session() { return static_pointer_cast<Session>(shared_from_this()); }

	RecvBuffer			get_recv_buffer() { return _recv_buffer; }

	void				get_account_idx(uint64 idx) { _account_idx = idx; }
	uint64				get_account_idx() { return _account_idx; }

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
	virtual int32		on_recv(BYTE* get_buffer, int32 len);
	virtual void		on_send(int32 len) { }
	virtual void		on_recv_packet(BYTE* get_buffer, int32 len);

private:
	SOCKET				_socket = INVALID_SOCKET;
	NetAddress			_net_address = {};
	atomic<bool>		_is_connect = false;
	uint64				_account_idx = 0;

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
};

