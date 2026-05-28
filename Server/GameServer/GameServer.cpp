#include "pch.h"
#include "../ServerCore/NetWork/Service.h"
#include "../ServerCore/NetWork/IocpCore.h"
#include "../ServerCore/DB/DBManager.h"
#include "../ServerCore/Thread/ThreadManager.h"
#include "../ServerCore/Utils/Logger.h"

#include "GameSession.h"
#include "ServerPacketHandler.h"
#include "PlayerManager.h"
#include "GameGlobal.h"

void do_worker_job(ServerServiceRef& service)
{
	while (true)
	{
		LEndTickCount = ::GetTickCount64() + 64;

		// 네트워크 입출력 처리 -> 인게임 로직까지 (패킷 핸들러에 의해)
		service->get_iocp_core()->dispatch(10);

		// 예약된 일감 처리
		ThreadManager::distribute_reserved_jobs();

		// 글로벌 큐
		ThreadManager::do_global_queue_work();

	}
}


int main()
{
	ServerPacketHandler::Init();

	GLogger->init("./Log");

	ServerServiceRef service = make_shared<ServerService>(
		NetAddress(L"127.0.0.1", 7777),
		make_shared<IocpCore>(),
		[=]() {return make_shared<GameSession>(); },
		make_shared<SessionManager>(100));

	ASSERT_CRASH(service->start());
	////ASSERT_CRASH(GDBManager->DBConnectStart());

	

	for (int32 i = 0; i < 5; i++)
	{
		GThreadManager->launch([&service]()
			{
				do_worker_job(service);
			});
	}

	GThreadManager->join();
}