#pragma once

#include "../Utils/Types.h"
#include "../NetWork/ClientServer.h"
#include "../Session/TestSession.h"
#include "../Packet/ClientPacketHandler.h"
#include "../Utils/GlobalStruct.h"
#include "../Utils/ClientConfig.h"


void run_load_test();
void run_manual_test();
void print_load_test_stats_loop(atomic<bool>& running);

int main()
{
	std::locale::global(std::locale(""));
	wcin.imbue(std::locale());
	wcout.imbue(std::locale());

	ClientPacketHandler::Init();

	WSADATA wsa_data;
	if (::WSAStartup(MAKEWORD(2, 2), OUT & wsa_data))
	{
		cout << "WSAStartup failed : " << WSAGetLastError() << endl;
		WSACleanup();
		return 0;
	}

	GClientMode = ClientMode::LOAD_TEST;

	cout << "1. Manual Test" << endl;
	cout << "2. Load Test" << endl;
	cout << "Select Mode : ";

	int32 input = 0;
	cin >> input;

	if (input == 1)
		GClientMode = ClientMode::MANUAL;

	if (GClientMode == ClientMode::MANUAL)
		run_manual_test();
	else
		run_load_test();

	WSACleanup();
	return 0;
}

void run_load_test()
{
	LoadTestConfig config;

	ClientServer server;
	if (!server.CreateIocpHandle())
		return;

	server.start_worker_thread(config.worker_thread_count);

	SOCKADDR_IN server_addr = server.start_server(config.ip, config.port);

	vector<shared_ptr<TestSession>> sessions;
	sessions.reserve(config.session_count);

	for (int32 i = 0; i < config.session_count; i++)
	{
		shared_ptr<TestSession> session = make_shared<TestSession>();
		session->set_bot_index(i);

		if (!session->connect(server_addr))
			continue;

		if (!server.register_socket(session))
			continue;

		if (!session->start())
			continue;

		session->test_login(i, false);
		sessions.push_back(session);
	}

	atomic<bool> running = true;

	vector<thread> bot_threads;
	bot_threads.reserve(config.bot_thread_count);

	for (int32 i = 0; i < config.bot_thread_count; i++)
	{
		bot_threads.push_back(thread([&, i]()
		{
			vector<shared_ptr<TestSession>> my_sessions;

			for (int32 index = i; index < static_cast<int32>(sessions.size()); index += config.bot_thread_count)
			{
				my_sessions.push_back(sessions[index]);
			}

			while (running)
			{
				for (shared_ptr<TestSession>& session : my_sessions)
				{
					if (session->get_is_login())
					{
						session->send_chat(L"bot test message", Protocol::CHAT_STATE::CHAT_ALL);
					}
				}

				::Sleep(config.chat_interval_ms);
			}
		}));
	}

	print_load_test_stats_loop(running);

	running = false;

	for (thread& bot_thread : bot_threads)
	{
		if (bot_thread.joinable())
			bot_thread.join();
	}

	server.stop();
}

void run_manual_test()
{
	ClientServer server;
	if (!server.CreateIocpHandle())
		return;

	server.start_worker_thread(1);

	SOCKADDR_IN server_addr = server.start_server(L"127.0.0.1", 7777);

	shared_ptr<TestSession> session = make_shared<TestSession>();

	if (!session->connect(server_addr))
		return;

	if (!server.register_socket(session))
		return;

	if (!session->start())
		return;

	session->login();

	wcout << L" /chat = 대화, /q = 종료" << endl;

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
	}

	server.stop();
}

void print_load_test_stats_loop(atomic<bool>& running)
{
	uint64 prev_send = 0;
	uint64 prev_recv = 0;
	auto prev_time = chrono::steady_clock::now();

	while (running)
	{
		auto now = chrono::steady_clock::now();
		double elapsed_sec = chrono::duration<double>(now - prev_time).count();
		prev_time = now;

		uint64 connect = GServerStats.connect_success.load();
		uint64 login = GServerStats.login_success.load();
		uint64 send = GServerStats.send_chat.load();
		uint64 recv = GServerStats.recv_chat.load();
		uint64 expected_recv = GServerStats.expected_chat_recv.load();
		uint64 disconnect = GServerStats.disconnect.load();

		uint64 send_delta = send - prev_send;
		uint64 recv_delta = recv - prev_recv;

		prev_send = send;
		prev_recv = recv;

		double send_per_sec = send_delta / elapsed_sec;
		double recv_per_sec = recv_delta / elapsed_sec;

		uint64 backlog = 0;
		if (expected_recv > recv)
			backlog = expected_recv - recv;

		wcout << L"connect = " << connect
			<< L" login = " << login
			<< L" send = " << send
			<< L" recv = " << recv
			<< L" disconnect = " << disconnect
			<< L" send/s = " << static_cast<uint64>(send_per_sec)
			<< L" recv/s = " << static_cast<uint64>(recv_per_sec)
			<< L" backlog = " << backlog
			<< endl;

		::Sleep(1000);
	}
}