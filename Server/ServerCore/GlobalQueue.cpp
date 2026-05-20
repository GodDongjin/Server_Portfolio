#include "pch.h"
#include "GlobalQueue.h"

GlobalQueue::GlobalQueue()
{

}

GlobalQueue::~GlobalQueue()
{

}

void GlobalQueue::push(JobQueueRef jobQueue)
{
	_jobQueues.push(jobQueue);
}

JobQueueRef GlobalQueue::pop()
{
	return _jobQueues.pop();
}