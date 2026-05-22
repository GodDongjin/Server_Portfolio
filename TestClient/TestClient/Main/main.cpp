#pragma once

#include "../Utils/Types.h"
#include "../NetWork/ClientServer.h"
#include "../Session/TestSession.h"

int main()
{
	WSADATA wsaData;
	if (::WSAStartup(MAKEWORD(2, 2), OUT & wsaData))
	{
		cout << "WSAStartup failed : " << WSAGetLastError() << endl;
		WSACleanup();
		return false;
	}

	ClientServer server;
	SOCKADDR_IN server_addr;
	server_addr = server.start_server(L"127.0.0.1", 7777);

	vector<shared_ptr<TestSession>> sessions;

	for (int i = 0; i < 1000; i++)
	{
		auto session = make_shared<TestSession>();

		if (!session->connect(server_addr))
			continue;

		sessions.push_back(session);
	}

	while (true)
	{
		::Sleep(1000);
	}

	return 0;
}