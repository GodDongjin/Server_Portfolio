#pragma once

#include "../Utils/Types.h"
#include "../NetWork/ClientServer.h"
#include "../Session/TestSession.h"
#include "../Packet/ClientPacketHandler.h"

int main()
{
	std::locale::global(std::locale(""));
	wcin.imbue(std::locale());
	wcout.imbue(std::locale());

	ClientPacketHandler::Init();

	WSADATA wsaData;
	if (::WSAStartup(MAKEWORD(2, 2), OUT & wsaData))
	{
		cout << "WSAStartup failed : " << WSAGetLastError() << endl;
		WSACleanup();
		return false;
	}

	ClientServer server;
	if (!server.CreateIocpHandle())
	{
		cout << "CreateIocpHandle Error " << endl;
		return 0;
	}

	server.start_worker_thread(4);

	SOCKADDR_IN server_addr;
	server_addr = server.start_server(L"127.0.0.1", 7777);

	vector<shared_ptr<TestSession>> sessions;

	
	// Bot test 코드
	///////////////////////////////////////////////////////////
	atomic<bool> running = true;
	const int32 session_Count = 1000;
	const int32 bot_thread_Count = 4;

	for (int32 i = 0; i < session_Count; i++)
	{
		auto session = make_shared<TestSession>();
		session->set_bot_index(i);

		if (!session->connect(server_addr)) {
			continue;
		}
			

		if (!server.register_socket(session)){
			continue;
		}
			
		if (!session->start()) {
			continue;
		}

		session->test_login(i);

		sessions.push_back(session);
	}
	
	vector<thread> bot_threads;
	for (int32 i = 0; i < bot_thread_Count; i++)
	{
		bot_threads.push_back(thread([&, i]()
		{
			vector<shared_ptr<TestSession>> mySessions;
			for (int32 index = i; index < sessions.size(); index += bot_thread_Count)
			{
				mySessions.push_back(sessions[index]);
			}

			while (running)
			{
				for (int32 t = 0; t < mySessions.size(); t++)
				{
					auto& session = mySessions[t];

					if (session->get_is_login())
					{
						session->send_chat(L"bot test message", Protocol::CHAT_STATE::CHAT_ALL);
					}
				}

				::Sleep(2000);
			}
		}));
	}

	while (true)
	{
		::Sleep(1000);
	}

	running = false;

	for (thread& t : bot_threads)
	{
		if (t.joinable())
			t.join();
	}
	/////////////////////////////////////////////////////////////////////


	/// 직접 테스트 할 때
	/*if (!session->connect(server_addr))
		return 0;

	if (!server.register_socket(session))
	{
		//에러 Log 적제
		cout << "register_socket ERROR" << endl;
		return 0;
	}

	if (!session->start())
	{
		cout << "session start ERROR" << endl;
		return 0;
	}

	session->login();

	while (true)
	{
		//cout << "[ /chat : 채팅, /q 종료  ]" << endl;
		//wcin.ignore(std::numeric_limits<std::streamsize>::max(), L'\n');
		wstring line;
		getline(wcin, line);

		if (line == L"/q")
		{
			session->logout();
			break;
		}

		if (line.rfind(L"/chat ", 0) == 0)
		{
			session->send_chat(line.substr(6), Protocol::CHAT_STATE::CHAT_ALL);
			continue;
		}

		cout << "unknown command" << endl;
	}

	while (session->is_connected())
	{
		::Sleep(10);
	}
	*//////////


	server.stop();
	WSACleanup();

	return 0;
}