#pragma once
#include "IocpObject.h"
#include "IocpEvent.h"

class JobQueue;

class Listener : public IocpObject
{
	enum
	{
		ACCEPT_RETRY_INTERVAL_MS = 100,
	};

public:
	Listener();
	~Listener();

public:
	bool start_accept(ServerServiceRef service);
	void close_socket();
	virtual bool is_closing() const override { return _accept_stopped.load(); }

public:
	virtual HANDLE get_handle() override;
	virtual void dispatch(class IocpEvent* iocpEvent, int32 numOfBytes = 0) override;

private:
	void register_accept(AcceptEvent* acceptEvent);
	void process_accept(AcceptEvent* acceptEvent);
	void reserve_accept_retry(AcceptEvent* acceptEvent);

private:
	// Accept 등록과 소켓 종료는 같은 잠금으로 보호한다.
	USE_LOCK;
	atomic<bool> _accept_stopped = false;
	shared_ptr<JobQueue> _accept_retry_queue;

protected:
	SOCKET _socket = INVALID_SOCKET;
	vector<AcceptEvent*> _accept_events;
	ServerServiceRef _service;
};

