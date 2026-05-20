#pragma once

struct JobData
{
	JobData(weak_ptr<JobQueue> owner, JobRef job) : _owner(owner), _job(job)
	{

	}

	weak_ptr<JobQueue>	_owner;
	JobRef				_job;
};

struct TimerItem
{
	bool operator<(const TimerItem& other) const
	{
		return _execute_tick > other._execute_tick;
	}

	uint64 _execute_tick = 0;
	JobData* _job_data = nullptr;
};

/*--------------
	JobTimer
---------------*/

class JobTimer
{
public:
	void			reserve(uint64 tickAfter, weak_ptr<JobQueue> owner, JobRef job);
	void			distribute(uint64 now);
	void			clear();

private:
	USE_LOCK;
	priority_queue<TimerItem>	_items;
	atomic<bool>				_distributing = false;
};


