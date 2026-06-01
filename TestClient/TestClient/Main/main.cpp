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

	auto session = make_shared<TestSession>();

	if (!session->connect(server_addr))
		return 0;

	if (!server.register_socket(session))
	{
		//俊矾 Log 利力
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

	//for (int i = 0; i < 1000; i++)
	//{
	//	auto session = make_shared<TestSession>();

	//	if (!session->connect(server_addr))
	//		continue;

	//	if (!server.register_socket(session))
	//	{
	//		//俊矾 Log 利力
	//		cout << "register_socket ERROR" << endl;
	//		continue;
	//	}

	//	if (!session->start())
	//	{
	//		cout << "session start ERROR" << endl;
	//		continue;
	//	}

	//	sessions.push_back(session);
	//}

	//for (shared_ptr<TestSession>& session : sessions)
	//{
	//	session->send();
	//}

	while (session->is_connected())
	{
		::Sleep(10);
	}

	server.stop();
	WSACleanup();

	return 0;
}