#pragma once

class IocpObject : public enable_shared_from_this<IocpObject>
{
public:
	virtual HANDLE get_handle() abstract;
	virtual void dispatch(class IocpEvent* iocpEvent, int32 numOfBytes = 0) abstract;
};