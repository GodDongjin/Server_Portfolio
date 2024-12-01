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
	void				Send(SendBufferRef sendBuffer);
	void				Disconnect(const WCHAR* cause);
	void				SetProcessConnect() { this->ProcessConnect(); }
public:
	//shared_ptr<Service>	GetService() { return mService.lock(); }
	//void				SetService(shared_ptr<Service> service) { mService = service; }
	void				SetNetAddress(NetAddress address) { mNetAddress = address; }
	NetAddress			GetAddress() { return mNetAddress; }
	SOCKET				GetSocket() { return mSocket; }
	bool				IsConnected() { return mConnected; }
	SessionRef			GetSessionRef() { return static_pointer_cast<Session>(shared_from_this()); }
	RecvBuffer			GetRecvBuffer() { return mRecvBuffer; }
	void				SetAccountIdx(uint64 idx) { mAccountIdx = idx; }
	uint64				GetAccountIdx() { return mAccountIdx; }

private:
	virtual HANDLE		GetHandle() override;
	virtual void		Dispatch(class IocpEvent* iocpEvent, int32 numOfBytes = 0) override;

private:
	//bool				RegisterConnect();
	bool				RegisterDisconnect();
	void				RegisterRecv();
	void				RegisterSend();

	void				ProcessConnect();
	void				ProcessDisconnect();
	void				ProcessRecv(int32 numOfBytes);
	void				ProcessSend(int32 numOfBytes);

	void				HandleError(int32 errorCode);

protected:
	virtual int32		OnRecv(BYTE* buffer, int32 len);
	virtual void		OnSend(int32 len) { }
	virtual void		OnRecvPacket(BYTE* buffer, int32 len);

private:
	//weak_ptr<Service>	mService;
	SOCKET				mSocket = INVALID_SOCKET;
	NetAddress			mNetAddress = {};
	atomic<bool>		mConnected = false;
	uint64				mAccountIdx = 0;
private:
	USE_LOCK;
	RecvBuffer				mRecvBuffer;

	queue<SendBufferRef>	mSendQueue;
	atomic<bool>			mSendRegistered = false;

private:
	ConnectEvent		mConnectEvent;
	DisconnectEvent		mDisconnectEvent;
	RecvEvent			mRecvEvent;
	SendEvent			mSendEvent;
};

