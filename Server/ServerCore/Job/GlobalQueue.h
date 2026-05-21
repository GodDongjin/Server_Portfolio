#pragma once
#include "LockQueue.h"

class GlobalQueue
{
public:
	GlobalQueue();
	~GlobalQueue();

	void					push(JobQueueRef jobQueue);
	JobQueueRef				pop();

private:
	LockQueue<JobQueueRef> _jobQueues;
};

