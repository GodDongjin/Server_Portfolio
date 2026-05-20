#pragma once

class IocpCore
{
public:
	IocpCore();
	~IocpCore();

	HANDLE		get_handle() { return _iocp_handle; }

	bool		Register(IocpObjectRef iocpObject);
	bool		dispatch(uint32 timeoutMs = INFINITE);

private:
	HANDLE		_iocp_handle;
};

