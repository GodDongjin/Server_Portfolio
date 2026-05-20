#include "pch.h"
#include "JobTimer.h"
#include "JobQueue.h"

/*--------------
	JobTimer
---------------*/

void JobTimer::reserve(uint64 tickAfter, weak_ptr<JobQueue> owner, JobRef job)
{
	const uint64 _execute_tick = ::GetTickCount64() + tickAfter;
	JobData* _job_data = new JobData(owner, job);

	WRITE_LOCK;

	_items.push(TimerItem{ _execute_tick, _job_data });
}

void JobTimer::distribute(uint64 now)
{
	// 한 번에 1 쓰레드만 통과
	if (_distributing.exchange(true) == true)
		return;

	vector<TimerItem> items;

	{
		WRITE_LOCK;

		while (_items.empty() == false)
		{
			const TimerItem& timerItem = _items.top();
			if (now < timerItem._execute_tick)
				break;

			items.push_back(timerItem);
			_items.pop();
		}
	}

	for (TimerItem& item : items)
	{
		if (JobQueueRef owner = item._job_data->_owner.lock())
			owner->push(item._job_data->_job);

		delete item._job_data;
	}

	// 끝났으면 풀어준다
	_distributing.store(false);
}

void JobTimer::clear()
{
	WRITE_LOCK;

	while (_items.empty() == false)
	{
		const TimerItem& timerItem = _items.top();
		delete timerItem._job_data;
		_items.pop();
	}
}

