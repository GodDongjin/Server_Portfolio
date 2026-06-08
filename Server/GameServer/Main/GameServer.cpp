#include "pch.h"
#include "../ServerCore/NetWork/Service.h"
#include "../ServerCore/NetWork/IocpCore.h"
#include "../ServerCore/DB/DBManager.h"
#include "../ServerCore/Thread/ThreadManager.h"
#include "../ServerCore/Utils/Logger.h"
#include "../ServerCore/Utils/ConfigReader.h"

#include "GameSession.h"
#include "../Packet/ServerPacketHandler.h"
#include "../Utils/GameGlobal.h"

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
	std::locale::global(std::locale(""));
	wcin.imbue(std::locale());
	wcout.imbue(std::locale());

	ServerPacketHandler::Init();
	ConfigReader config(L"Config/config.ini");

	if (!config.load())
	{
		wcerr << L"Failed to load config.ini" << endl;
		return 0;
	}

	const wstring server_ip = config.get_wstring("server", "ip");
	const uint16 port_value = static_cast<uint16>(config.get_int("server", "port"));
	const int32 session_max_count = config.get_int("server", "session_max_count");
	const int32 worker_thread_count = config.get_int("server", "work_thread_count");

	if (port_value <= 0 || port_value > 65535)
	{
		wcerr << L"Invalid server port: " << port_value << endl;
		return 0;
	}

	if (session_max_count <= 0)
	{
		wcerr << L"Invalid session max count: " << session_max_count << endl;
		return 0;
	}

	if (worker_thread_count <= 0)
	{
		wcerr << L"Invalid worker thread count: " << worker_thread_count << endl;
		return 0;
	}

	GLogger->init("./Log");

	ServerServiceRef service = make_shared<ServerService>(
		NetAddress(server_ip, port_value),
		make_shared<IocpCore>(),
		[=]() {return make_shared<GameSession>(); },
		make_shared<SessionManager>(session_max_count));

	ASSERT_CRASH(service->start());

	// DB 시스템 구정 되면 사용
	/*ASSERT_CRASH(GDBManager->db_connect_start(
		config.get_wstring("database_info", "driver"),
		config.get_wstring("database_info", "ip"),
		config.get_wstring("database_info", "database"),
		config.get_wstring("database_info", "user"),
		config.get_wstring("database_info", "password"))
	);*/

	for (int32 i = 0; i < worker_thread_count; i++)
	{
		GThreadManager->launch([&service]()
			{
				do_worker_job(service);
			});
	}

	GThreadManager->launch([]()
	{
		uint64 prevRecvChat = 0;
		uint64 prevBroadcastTarget = 0;

		while (true)
		{
			uint64 recvChat = GServerStats.recv_chat.load();
			uint64 broadcastTarget = GServerStats.broadcast_target.load();

			uint64 recvChatDelta = recvChat - prevRecvChat;
			uint64 broadcastDelta = broadcastTarget - prevBroadcastTarget;

			prevRecvChat = recvChat;
			prevBroadcastTarget = broadcastTarget;

			wcout << L"[SERVER] "
				<< L"connect = " << GServerStats.connect.load()
				<< L" login = " << GServerStats.login.load()
				<< L" recv_chat = " << recvChat
				<< L" recv_chat/s = " << recvChatDelta
				<< L" broadcast_target = " << broadcastTarget
				<< L" broadcast/s = " << broadcastDelta
				<< L" send_complete = " << GServerStats.send_complete.load()
				<< L" send_bytes = " << GServerStats.send_complete_bytes.load()
				<< L" disconnect = " << GServerStats.disconnect.load()
				<< endl;

			::Sleep(1000);
		}
	});

	GThreadManager->join();
}