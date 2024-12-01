#include "pch.h"
#include "ThreadManager.h"
#include "Service.h"
#include "GameSession.h"
#include "IocpCore.h"
#include "ServerPacketHandler.h"
#include "DBManager.h"
#include "PlayerManager.h"

void DoWorkerJob(ServerServiceRef& service)
{
	while (true)
	{
		LEndTickCount = ::GetTickCount64() + 64;

		// ��Ʈ��ũ ����� ó�� -> �ΰ��� �������� (��Ŷ �ڵ鷯�� ����)
		service->GetIocpCore()->Dispatch(10);

		// ����� �ϰ� ó��
		ThreadManager::DistributeReservedJobs();

		// �۷ι� ť
		ThreadManager::DoGlobalQueueWork();

	}
}


int main()
{
	ServerPacketHandler::Init();

	ServerServiceRef service = make_shared<ServerService>(
		NetAddress(L"127.0.0.1", 7777),
		make_shared<IocpCore>(),
		[=]() {return make_shared<GameSession>(); },
		make_shared<SessionManager>(100));

	ASSERT_CRASH(service->Start());
	ASSERT_CRASH(GDBManager->DBConnectStart());

	for (int32 i = 0; i < 5; i++)
	{
		GThreadManager->Launch([&service]()
			{
				DoWorkerJob(service);
			});
	}

	GThreadManager->Join();
}