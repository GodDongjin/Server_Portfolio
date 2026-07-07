#pragma once

#include "../Utils/Types.h"
#include "../NetWork/ClientServer.h"
#include "../Session/TestSession.h"
#include "../Packet/ClientPacketHandler.h"
#include "../Utils/GlobalData.h"
#include "../Utils/ConfigReader.h"

void run_load_test(ConfigReader& config);
void run_manual_test(ConfigReader& config);
void print_load_test_stats_loop(atomic<bool>& running);

int main()
{
	std::locale::global(std::locale(""));
	wcin.imbue(std::locale());
	wcout.imbue(std::locale());

	ConfigReader config(L"Config/config.ini");
	if (!config.load())
	{
		wcerr << L"Failed to load config.ini" << endl;
		return 0;
	}

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
		run_manual_test(config);
	else
		run_load_test(config);

	WSACleanup();
	return 0;
}

void run_load_test(ConfigReader& config)
{
	ClientServer server;
	if (!server.CreateIocpHandle())
		return;

	const wstring server_ip = config.get_wstring("server", "ip");
	const uint16 port_value = static_cast<uint16>(config.get_int("server", "port"));
	const int32 session_count = config.get_int("client", "session_count");
	const int32 worker_thread_count = config.get_int("client", "worker_thread_count");
	const int32 bot_thread_count = config.get_int("client", "bot_thread_count");
	const int32 chat_interval_ms = config.get_int("client", "chat_interval_ms");
	const int32 bot_room_count = max(1, config.get_int("client", "bot_room_count", 10));
	const int32 all_chat_interval = max(1, config.get_int("client", "all_chat_interval", 10));
	const int32 whisper_interval = max(1, config.get_int("client", "whisper_interval", 15));
	const int32 bot_room_target_count = max(1, config.get_int("client", "bot_room_target_count", 10));
	GExpectedRoomRecvCount.store(bot_room_target_count);

	if (port_value <= 0 || port_value > 65535)
	{
		wcerr << L"Invalid server port: " << port_value << endl;
		return;
	}

	server.start_worker_thread(worker_thread_count);

	SOCKADDR_IN server_addr = server.start_server(server_ip, port_value);

	vector<shared_ptr<TestSession>> sessions;
	sessions.reserve(session_count);

	for (int32 i = 0; i < session_count; i++)
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
	bot_threads.reserve(bot_thread_count);

	for (int32 i = 0; i < bot_thread_count; i++)
	{
		bot_threads.push_back(thread([&, i]()
		{
			vector<shared_ptr<TestSession>> my_sessions;

			for (int32 index = i; index < static_cast<int32>(sessions.size()); index += bot_thread_count)
			{
				my_sessions.push_back(sessions[index]);
			}

			while (running)
			{
				for (shared_ptr<TestSession>& session : my_sessions)
				{
					if (session->get_is_login() == false)
						continue;

					const int32 bot_index = session->get_bot_index();

					if (session->get_room_id() < 0)
					{
						session->send_enter_room(static_cast<uint8>(bot_index % bot_room_count));
						continue;
					}

					if (bot_index % all_chat_interval == 0)
					{
						session->send_chat(L"bot all chat test", Protocol::CHAT_STATE::CHAT_ALL, session->get_room_id());
						continue;
					}

					if (sessions.size() > 1 && bot_index % whisper_interval == 0)
					{
						const int32 target_index = (bot_index + 1) % static_cast<int32>(sessions.size());
						const wstring target_name = L"Bot_" + to_wstring(target_index);
						session->send_whisper_chat(target_name, L"bot whisper test", Protocol::CHAT_STATE::CHAT_WHISPER, session->get_room_id());
						continue;
					}

					session->send_chat(L"bot room chat test", Protocol::CHAT_STATE::CHAT_NORMAL, session->get_room_id());
				}

				::Sleep(chat_interval_ms);
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

void run_manual_test(ConfigReader& config)
{
	ClientServer server;
	if (!server.CreateIocpHandle())
		return;

	const wstring server_ip = config.get_wstring("server", "ip");
	const uint16 port_value = static_cast<uint16>(config.get_int("server", "port"));

	if (port_value <= 0 || port_value > 65535)
	{
		wcerr << L"Invalid server port: " << port_value << endl;
		return;
	}

	server.start_worker_thread(1);

	SOCKADDR_IN server_addr = server.start_server(server_ip, port_value);

	shared_ptr<TestSession> session = make_shared<TestSession>();

	if (!session->connect(server_addr))
		return;

	if (!server.register_socket(session))
		return;

	if (!session->start())
		return;

	session->login();

	wcout << L" /rooms = 방 목록, /enter [room_id] = 방 입장, /exit = 방 퇴장, /chat [message] = 방 채팅, /all [message] = 전체 채팅, /whisper [name] [message] = 귓속말, /q = 종료" << endl;

	while (true)
	{
		wstring line;
		getline(wcin, line);

		if (line == L"/q")
		{
			session->logout();
			break;
		}

		if (line == L"/rooms")
		{
			session->send_get_room_info();
			continue;
		}

		if (line.rfind(L"/enter ", 0) == 0)
		{
			try
			{
				int32 room_id = stoi(line.substr(7));

				if (room_id < 0 || room_id > 255)
				{
					wcout << L"invalid room id" << endl;
					continue;
				}

				session->send_enter_room(static_cast<uint8>(room_id));
			}
			catch (...)
			{
				wcout << L"invalid room id" << endl;
			}

			continue;
		}

		if (line == L"/exit")
		{
			session->send_exit_room();
			continue;
		}

		if (line.rfind(L"/all ", 0) == 0)
		{
			session->send_chat(line.substr(5), Protocol::CHAT_STATE::CHAT_ALL, session->get_room_id());
			continue;
		}

		if (line.rfind(L"/chat ", 0) == 0)
		{
			session->send_chat(line.substr(6), Protocol::CHAT_STATE::CHAT_NORMAL, session->get_room_id());
			continue;
		}

		if (line.rfind(L"/whisper ", 0) == 0)
		{
			wstring command_body = line.substr(10);
			size_t split_pos = command_body.find(L' ');

			if (split_pos == wstring::npos)
			{
				wcout << L"usage: /whisper [name] [message]" << endl;
				continue;
			}

			wstring target_name = command_body.substr(0, split_pos);
			wstring message = command_body.substr(split_pos + 1);

			if (target_name.empty() || message.empty())
			{
				wcout << L"usage: /whisper [name] [message]" << endl;
				continue;
			}

			session->send_whisper_chat(target_name, message, Protocol::CHAT_STATE::CHAT_WHISPER, session->get_room_id());
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










