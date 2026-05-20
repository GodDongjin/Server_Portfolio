#pragma once

enum class EventType : uint8
{
	connect,
	disconnect,
	Accept,
	Recv,
	send
};

class IocpEvent : public OVERLAPPED
{
public:
	IocpEvent(EventType type);
	void			init();

public:
	EventType get_event_type() { return _event_type; }
	IocpObjectRef get_owner() { return _owner; }
	void set_owner(IocpObjectRef owner) { _owner = owner; }

private:
	EventType		_event_type;
	IocpObjectRef	_owner;
};

/*----------------
	ConnectEvent
-----------------*/

class ConnectEvent : public IocpEvent
{
public:
	ConnectEvent() : IocpEvent(EventType::connect) { }
};

/*--------------------
	DisconnectEvent
----------------------*/

class DisconnectEvent : public IocpEvent
{
public:
	DisconnectEvent() : IocpEvent(EventType::disconnect) { }
};

/*----------------
	AcceptEvent
-----------------*/

class AcceptEvent : public IocpEvent
{
public:
	AcceptEvent() : IocpEvent(EventType::Accept) { }

public:
	SessionRef get_session() { return _session; }
	void set_session(SessionRef session) { _session = session; }

private:
	SessionRef	_session = nullptr;
};

/*----------------
	RecvEvent
-----------------*/

class RecvEvent : public IocpEvent
{
public:
	RecvEvent() : IocpEvent(EventType::Recv) { }
};

/*----------------
	SendEvent
-----------------*/

class SendEvent : public IocpEvent
{
public:
	SendEvent() : IocpEvent(EventType::send) { }

public:
	vector<SendBufferRef> get_send_Buffers() { return _send_Buffers; }
	void send_Buffers_push(SendBufferRef sendBuffer) { _send_Buffers.push_back(sendBuffer); }
	void send_Buffers_clear() { _send_Buffers.clear(); }
private:
	vector<SendBufferRef> _send_Buffers;
};