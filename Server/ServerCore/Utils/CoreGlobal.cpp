#include "pch.h"
#include "CoreGlobal.h"
#include "ThreadManager.h"
#include "GlobalQueue.h"
#include "JobTimer.h"
#include "SocketUtils.h"
#include "DBManager.h"
#include "Logger.h"

ThreadManager* GThreadManager = nullptr;
GlobalQueue* GGlobalQueue = nullptr;
JobTimer* GJobTimer = nullptr;
DBManager* GDBManager = nullptr;
Logger* GLogger = nullptr;

class CoreGlobal
{
public:
	CoreGlobal()
	{
		GThreadManager = new ThreadManager();
		GGlobalQueue = new GlobalQueue();
		GJobTimer = new JobTimer();
		GDBManager = new DBManager();
		GLogger = new Logger();
		SocketUtils::init();
	}

	~CoreGlobal()
	{
		delete GThreadManager;
		delete GGlobalQueue;
		delete GJobTimer;
		delete GDBManager;
		delete GLogger;
		SocketUtils::clear();
	}
} GCoreGlobal;