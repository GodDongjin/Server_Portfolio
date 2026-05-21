#include "pch.h"
#include "ThreadManager.h"
#include "CoreTLS.h"
#include "CoreGlobal.h"
#include "GlobalQueue.h"
#include "JobQueue.h"

ThreadManager::ThreadManager()
{
	// Main Thread
	init_TLS();
}

ThreadManager::~ThreadManager()
{
	join();
}

void ThreadManager::launch(function<void(void)> callback)
{
	lock_guard<mutex> guard(_lock);

	_threads.push_back(thread([=]()
		{
			init_TLS();
			callback();
			destroy_TLS();
		}));
}

void ThreadManager::join()
{
	for (thread& t : _threads)
	{
		if (t.joinable())
			t.join();
	}
	_threads.clear();
}

void ThreadManager::init_TLS()
{
	static atomic<uint32> SThreadId = 1;
	LThreadId = SThreadId.fetch_add(1);
}

void ThreadManager::destroy_TLS()
{

}

void ThreadManager::do_global_queue_work()
{
	while (true)
	{
		uint64 now = ::GetTickCount64();
		if (now > LEndTickCount)
			break;

		JobQueueRef jobQueue = GGlobalQueue->pop();
		if (jobQueue == nullptr)
			break;

		jobQueue->execute();
	}
}

void ThreadManager::distribute_reserved_jobs()
{
	const uint64 now = ::GetTickCount64();

	GJobTimer->distribute(now);
}
