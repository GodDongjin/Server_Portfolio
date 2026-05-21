#pragma once

#include <thread>
#include <functional>

class ThreadManager
{
public:
	ThreadManager();
	~ThreadManager();

	void	launch(function<void(void)> callback);
	void	join();

	static void init_TLS();
	static void destroy_TLS();

	static void do_global_queue_work();
	static void distribute_reserved_jobs();

private:
	mutex			_lock;
	vector<thread>	_threads;
};

