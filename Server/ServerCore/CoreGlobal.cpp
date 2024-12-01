#include "pch.h"
#include "CoreGlobal.h"
#include "ThreadManager.h"
#include "GlobalQueue.h"
#include "JobTimer.h"
#include "SocketUtils.h"
#include "DBManager.h"

ThreadManager* GThreadManager = nullptr;
GlobalQueue* GGlobalQueue = nullptr;
JobTimer* GJobTimer = nullptr;
DBManager* GDBManager = nullptr;

class CoreGlobal
{
public:
	CoreGlobal()
	{
		GThreadManager = new ThreadManager();
		GGlobalQueue = new GlobalQueue();
		GJobTimer = new JobTimer();
		GDBManager = new DBManager();
		SocketUtils::Init();
	}

	~CoreGlobal()
	{
		delete GThreadManager;
		delete GGlobalQueue;
		delete GJobTimer;
		delete GDBManager;
		SocketUtils::Clear();
	}
} GCoreGlobal;