#include "pch.h"
#include "IocpCore.h"
#include "IocpObject.h"
#include "IocpEvent.h"

IocpCore::IocpCore()
{
	_iocp_handle = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);
	ASSERT_CRASH(_iocp_handle != INVALID_HANDLE_VALUE);
}

IocpCore::~IocpCore()
{
	::CloseHandle(_iocp_handle);
}

bool IocpCore::Register(IocpObjectRef iocpObject)
{
	return ::CreateIoCompletionPort(iocpObject->get_handle(), _iocp_handle, /*key*/0, 0);
}

bool IocpCore::dispatch(uint32 timeoutMs)
{
	DWORD numOfBytes = 0;
	ULONG_PTR key = 0;
	IocpEvent* iocpEvent = nullptr;

	if (::GetQueuedCompletionStatus(_iocp_handle, OUT & numOfBytes, OUT & key, OUT reinterpret_cast<LPOVERLAPPED*>(&iocpEvent), timeoutMs))
	{
		if (iocpEvent == nullptr) {
			ERROR_LOG("GetQueuedCompletionStatus ERROR");
			return false;
		}

		IocpObjectRef iocpObject = iocpEvent->get_owner();
		iocpObject->dispatch(iocpEvent, numOfBytes);
	}
	else
	{
		int32 errCode = ::WSAGetLastError();
		switch (errCode)
		{
		case WAIT_TIMEOUT:
			return false;
		default:
			// TODO : ·Î±× Âï±â
			IocpObjectRef iocpObject = iocpEvent->get_owner();
			iocpObject->dispatch(iocpEvent, numOfBytes);
			break;
		}
	}

	return true;
}
