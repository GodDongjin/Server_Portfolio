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
	return ::CreateIoCompletionPort(iocpObject->get_handle(), _iocp_handle, /*key*/0, 0) != nullptr;
}

bool IocpCore::dispatch(uint32 timeoutMs)
{
	DWORD numOfBytes = 0;
	ULONG_PTR key = 0;
	IocpEvent* iocpEvent = nullptr;
	
	bool result = ::GetQueuedCompletionStatus(
		_iocp_handle, 
		OUT & numOfBytes, 
		OUT & key, 
		OUT reinterpret_cast<LPOVERLAPPED*>(&iocpEvent), 
		timeoutMs);

	if (!result)
	{
		const DWORD errCode = ::GetLastError();

		if (iocpEvent == nullptr)
		{
			if (errCode == WAIT_TIMEOUT) {
				return false;
			}

			ERROR_LOG("GetQueuedCompletionStatus failed");
			return false;
		}

		auto owner = iocpEvent->get_owner();
		// 서버가 직접 닫은 연결의 취소 통지는 오류로 기록하지 않는다.
		const bool shutdown_cancel = owner && owner->is_closing()
			&& (errCode == ERROR_OPERATION_ABORTED || errCode == ERROR_CONNECTION_ABORTED);
		if (!shutdown_cancel)
			ERROR_LOG("IO failed completion");
		// 실패한 완료 통지는 정상 송신 바이트로 처리하지 않고 연결 종료 경로로 보낸다.
		numOfBytes = 0;
	}

	if (iocpEvent == nullptr) {
		ERROR_LOG("GetQueuedCompletionStatus ERROR");
		return false;
	}

	IocpObjectRef iocpObject = iocpEvent->get_owner();
	if (iocpObject == nullptr)
	{
		ERROR_LOG("IocpEvent owner expired");
		return false;
	}

	iocpObject->dispatch(iocpEvent, numOfBytes);
	// 콜백이 새 IO나 정리 작업을 등록할 수 있으므로 콜백 반환 후 감소시킨다.
	iocpObject->finish_io();
	return true;
}
