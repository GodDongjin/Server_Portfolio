#pragma once
#include <condition_variable>
#include <mutex>

class IocpObject : public enable_shared_from_this<IocpObject>
{
public:
	virtual ~IocpObject() = default;
	virtual HANDLE get_handle() abstract;
	virtual void dispatch(class IocpEvent* iocpEvent, int32 numOfBytes = 0) abstract;
	virtual bool is_closing() const { return false; }

	// 등록 전에 증가시키고, 즉시 실패하거나 완료 콜백까지 처리한 뒤 감소시킨다.
	void begin_io()
	{
		lock_guard<mutex> guard(_completion_lock);
		++_pending_io;
	}

	void finish_io()
	{
		lock_guard<mutex> guard(_completion_lock);
		ASSERT_CRASH(_pending_io > 0);
		if (--_pending_io == 0)
			_io_completed.notify_all();
	}

	// 새 IO 등록을 막은 뒤, IOCP 작업 스레드가 아닌 종료 스레드에서 호출한다.
	void wait_for_io()
	{
		unique_lock<mutex> guard(_completion_lock);
		_io_completed.wait(guard, [this]() { return _pending_io == 0; });
	}

private:
	mutex _completion_lock;
	condition_variable _io_completed;
	uint32 _pending_io = 0;
};