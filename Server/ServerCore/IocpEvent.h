#pragma once

enum class EventType : uint8
{
	Connect,
	Disconnect,
	Accept,
	Recv,
	Send
};

class IocpEvent : public OVERLAPPED
{
public:
	IocpEvent(EventType type);
	void			Init();

public:
	EventType GetEventType() { return mEventType; }
	IocpObjectRef GetOwner() { return mOwner; }
	void SetOwner(IocpObjectRef owner) { mOwner = owner; }

private:
	EventType		mEventType;
	IocpObjectRef	mOwner;
};

/*----------------
	ConnectEvent
-----------------*/

class ConnectEvent : public IocpEvent
{
public:
	ConnectEvent() : IocpEvent(EventType::Connect) { }
};

/*--------------------
	DisconnectEvent
----------------------*/

class DisconnectEvent : public IocpEvent
{
public:
	DisconnectEvent() : IocpEvent(EventType::Disconnect) { }
};

/*----------------
	AcceptEvent
-----------------*/

class AcceptEvent : public IocpEvent
{
public:
	AcceptEvent() : IocpEvent(EventType::Accept) { }

public:
	SessionRef GetSession() { return mSession; }
	void SetSession(SessionRef session) { mSession = session; }

private:
	SessionRef	mSession = nullptr;
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
	SendEvent() : IocpEvent(EventType::Send) { }

public:
	vector<SendBufferRef> GetSendBuffer() { return mSendBuffers; }
	void SendBuffersPush(SendBufferRef sendBuffer) { mSendBuffers.push_back(sendBuffer); }
	void SendBuffersClear() { mSendBuffers.clear(); }
private:
	vector<SendBufferRef> mSendBuffers;
};