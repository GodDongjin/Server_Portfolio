#pragma once
#include "IocpObject.h"
#include "IocpEvent.h"

class Listener : public IocpObject
{
public:
	Listener() = default;
	~Listener();

public:
	bool start_accept(ServerServiceRef service);
	void close_socket();

public:
	virtual HANDLE get_handle() override;
	virtual void dispatch(class IocpEvent* iocpEvent, int32 numOfBytes = 0) override;

private:
	void register_accept(AcceptEvent* acceptEvent);
	void process_accept(AcceptEvent* acceptEvent);

protected:
	SOCKET _socket = INVALID_SOCKET;
	vector<AcceptEvent*> _accept_events;
	ServerServiceRef _service;
};

