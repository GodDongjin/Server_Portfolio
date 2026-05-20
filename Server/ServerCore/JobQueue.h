#pragma once
#include "Job.h"
#include "LockQueue.h"
#include "JobTimer.h"

class JobQueue : public enable_shared_from_this<JobQueue>
{
public:
	void do_async(CallbackType&& callback)
	{
		push(make_shared<Job>(std::move(callback)));
	}

	template<typename T, typename Ret, typename... Args>
	void do_async(Ret(T::* memFunc)(Args...), Args... args)
	{
		shared_ptr<T> owner = static_pointer_cast<T>(shared_from_this());
		push(make_shared<Job>(owner, memFunc, std::forward<Args>(args)...));
	}

	void do_timer(uint64 tickAfter, CallbackType&& callback)
	{
		JobRef job = make_shared<Job>(std::move(callback));
		GJobTimer->reserve(tickAfter, shared_from_this(), job);
	}

	template<typename T, typename Ret, typename... Args>
	void do_timer(uint64 tickAfter, Ret(T::* memFunc)(Args...), Args... args)
	{
		shared_ptr<T> owner = static_pointer_cast<T>(shared_from_this());
		JobRef job = make_shared<Job>(owner, memFunc, std::forward<Args>(args)...);
		GJobTimer->reserve(tickAfter, shared_from_this(), job);
	}

	void					clear_jobs() { _jobs.clear(); }

public:
	void					push(JobRef job, bool pushOnly = false);
	void					execute();

protected:
	LockQueue<JobRef>		_jobs;
	atomic<int32>			_jobCount = 0;
};

