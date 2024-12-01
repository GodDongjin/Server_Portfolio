#pragma once
#include "IocpObject.h"
#include "IocpEvent.h"

class Listener : public IocpObject
{
public:
	Listener() = default;
	~Listener();

public:
	bool StartAccept(ServerServiceRef service);
	void CloseSocket();

public:
	virtual HANDLE GetHandle() override;
	virtual void Dispatch(class IocpEvent* iocpEvent, int32 numOfBytes = 0) override;

private:
	void RegisterAccept(AcceptEvent* acceptEvent);
	void ProcessAccept(AcceptEvent* acceptEvent);

protected:
	SOCKET mSocket = INVALID_SOCKET;
	vector<AcceptEvent*> mAcceptEvents;
	ServerServiceRef mService;
};

