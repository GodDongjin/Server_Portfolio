#pragma once

#include "../Utils/Types.h"
#include "../Buffer/SendBuffer.h"

enum class EventType : uint8
{
	RECV,
	SEND
};

class IocpEvent : public OVERLAPPED
{
public:
	IocpEvent(EventType event_type) : _event_type(event_type)
	{
		init();
	}

	void init()
	{
		OVERLAPPED::hEvent = 0;
		OVERLAPPED::Internal = 0;
		OVERLAPPED::InternalHigh = 0;
		OVERLAPPED::Offset = 0;
		OVERLAPPED::OffsetHigh = 0;
	}

	EventType get_event_type() { return _event_type; }

protected:
	EventType _event_type;
};

class RecvEvent : public IocpEvent
{
public:
	RecvEvent() : IocpEvent(EventType::RECV) {}
};

class SendEvent : public IocpEvent
{
public:
	SendEvent() : IocpEvent(EventType::SEND) {}

	void clear() 
	{
		_send_Buffers.clear();
	}

public:
	const vector<shared_ptr<SendBuffer>> get_send_Buffers() { return _send_Buffers; }
	void send_Buffers_push(shared_ptr<SendBuffer> sendBuffer) { _send_Buffers.push_back(sendBuffer); }
	void send_Buffers_clear() { _send_Buffers.clear(); }

private:
	vector<shared_ptr<SendBuffer>> _send_Buffers;
};