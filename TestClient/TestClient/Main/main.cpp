#include "../Utils/Types.h"
#include "../NetWork/ClientServer.h"
#include "../Session/TestSession.h"
#include "../Packet/ClientPacketHandler.h"
#include "../Utils/GlobalData.h"
#include "../Utils/ClientConfig.h"
#include "../Utils/StringUtil.h"
#include <condition_variable>
#include <thread>

struct LoadControl
{
	atomic<bool> running = true;
	mutex lock;
	condition_variable changed;

	void wait(uint32 interval_ms)
	{
		unique_lock<mutex> guard(lock);
		changed.wait_for(guard, chrono::milliseconds(interval_ms),
			[this]() { return !running.load(); });
	}

	void stop()
	{
		lock_guard<mutex> guard(lock);
		running.store(false);
		changed.notify_all();
	}
};

bool run_load_test(ConfigReader& config);
bool run_manual_test(ConfigReader& config);
void print_load_test_stats_loop(LoadControl& control);

int main()
{
	std::locale::global(std::locale(""));
	wcin.imbue(std::locale());
	wcout.imbue(std::locale());

	ConfigReader config(L"Config/config.ini");
	if (!config.load())
	{
		wcerr << L"Failed to load config.ini" << endl;
		return 1;
	}
	ClientPacketHandler::Init();
	wcout << L"1. Manual Test\n2. Load Test\nSelect Mode : " << endl;
	wstring mode;
	if (!getline(wcin, mode)) {
		return 0;
	}
	if (mode != L"1" && mode != L"2")
	{
		wcerr << L"모드는 1 또는 2를 입력하세요." << endl;
		return 1;
	}
	GClientMode = mode == L"1" ? ClientMode::MANUAL : ClientMode::LOAD_TEST;

	WSADATA wsa_data{};
	const int32 error_code = ::WSAStartup(MAKEWORD(2, 2), &wsa_data);
	if (error_code != 0)
	{
		cerr << "WSAStartup failed : " << error_code << endl;
		return 1;
	}
	// Winsock은 프로세스에서 한 번 시작하고 모든 세션과 작업 스레드가 끝난 뒤 정리한다.
	struct WinsockScope
	{
		~WinsockScope() { ::WSACleanup(); }
	} winsock_scope;

	return (GClientMode == ClientMode::MANUAL ? run_manual_test(config) : run_load_test(config)) ? 0 : 1;
}

bool run_load_test(ConfigReader& config)
{
	ClientConfig settings;
	if (!settings.load(config, true)) {
		return false;
	}
	ClientServer server;
	SOCKADDR_IN server_addr{};
	if (!server.start_server(settings.server_ip, settings.port, server_addr) ||
		!server.CreateIocpHandle() || !server.start_worker_thread(settings.worker_thread_count)) {
		return false;
	}

	vector<shared_ptr<TestSession>> sessions;
	sessions.reserve(settings.session_count);
	for (int32 i = 0; i < settings.session_count; i++)
	{
		auto session = make_shared<TestSession>();
		session->set_bot_index(i);
		if (!session->connect(server_addr) || !server.register_socket(session) || !session->start()) {
			continue;
		}
		session->test_login(i, false);
		sessions.push_back(session);
	}

	LoadControl control;
	vector<thread> bot_threads;
	bot_threads.reserve(settings.bot_thread_count);
	for (int32 i = 0; i < settings.bot_thread_count; i++)
	{
		bot_threads.emplace_back([&, i]()
		{
			while (control.running.load())
			{
				for (size_t index = i; index < sessions.size(); index += settings.bot_thread_count)
				{
					const auto& session = sessions[index];
					if (!control.running.load()) {
						break;
					}
					if (!session->get_is_login()) {
						continue;
					}
					const int32 bot_index = session->get_bot_index();
					const int32 room_id = session->get_room_id();
					if (room_id < 0)
					{
						session->send_enter_room(static_cast<uint8>(bot_index % settings.bot_room_count));
						continue;
					}
					if (bot_index % settings.all_chat_interval == 0) {
						session->send_chat(L"bot all chat test", Protocol::CHAT_STATE::CHAT_ALL, room_id);
					}
					else if (sessions.size() > 1 && bot_index % settings.whisper_interval == 0)
					{
						// 연결 실패로 봇 번호에 빈칸이 있어도 실제 연결된 세션의 이름을 선택한다.
						const auto& target = sessions[(index + 1) % sessions.size()];
						session->send_whisper_chat(L"Bot_" + to_wstring(target->get_bot_index()),
							L"bot whisper test", Protocol::CHAT_STATE::CHAT_WHISPER, room_id);
					}
					else {
						session->send_chat(L"bot room chat test", Protocol::CHAT_STATE::CHAT_NORMAL, room_id);
					}
				}
				control.wait(settings.chat_interval_ms);
			}
		});
	}
	thread stats([&]() { print_load_test_stats_loop(control); });

	// 입력은 메인 스레드 한 곳에서만 읽는다. /q 또는 입력 종료로 부하 테스트도 정리한다.
	wcout << L"/q = 부하 테스트 종료" << endl;
	wstring line;
	while (getline(wcin, line)) {
		if (line == L"/q") {
			break;
		}
	}
	control.stop();
	for (thread& bot_thread : bot_threads) {
		bot_thread.join();
	}
	stats.join();
	server.stop();
	wcout << L"클라이언트 종료 완료" << endl;
	return true;
}

bool run_manual_test(ConfigReader& config)
{
	ClientConfig settings;
	if (!settings.load(config, false)) {
		return false;
	}
	ClientServer server;
	SOCKADDR_IN server_addr{};
	if (!server.start_server(settings.server_ip, settings.port, server_addr) ||
		!server.CreateIocpHandle() || !server.start_worker_thread(1)) {
		return false;
	}
	auto session = make_shared<TestSession>();
	if (!session->connect(server_addr) || !server.register_socket(session) || !session->start()) {
		return false;
	}

	while (session->is_connected() && !session->get_is_login())
	{
		wcout << L"1. 로그인    2. 계정 생성   3. 나가기" << endl;
		wstring choice, id, pw, name;
		if (!getline(wcin, choice) || choice == L"3" || choice == L"/q") {
			break;
		}
		if (choice != L"1" && choice != L"2")
		{
			wcout << L"1, 2, 3 중 하나를 입력하세요." << endl;
			continue;
		}
		wcout << L"id 입력 : " << endl;
		if (!getline(wcin, id)) {
			break;
		}
		wcout << L"password 입력 : " << endl;
		if (!getline(wcin, pw)) {
			break;
		}
		if (choice == L"2")
		{
			wcout << L"이름 입력 : " << endl;
			if (!getline(wcin, name)) {
				break;
			}
		}
		if (!session->login(choice == L"2", WStringToUtf8(id), WStringToUtf8(pw), name)) {
			continue;
		}
		// 메인 스레드가 결과를 기다리는 동안에도 IOCP 스레드는 핑과 수신을 계속 처리한다.
		if (!session->wait_for_login())
		{
			wcout << L"로그인 응답 시간이 초과되었습니다." << endl;
			session->disconnect();
		}
	}

	if (session->get_is_login()) {
		wcout << L"/rooms = 방 목록, /enter [room_id] = 입장, /exit = 퇴장, "
			L"/chat [message] = 방 채팅, /all [message] = 전체 채팅, "
			L"/whisper [name] [message] = 귓속말, /q = 종료" << endl;
	}
	wstring line;
	while (session->is_connected() && session->get_is_login() && getline(wcin, line))
	{
		if (line == L"/q")
		{
			session->logout();
			session->wait_for_disconnect();
			break;
		}
		if (line == L"/rooms") {
			session->send_get_room_info();
		}
		else if (line.rfind(L"/enter ", 0) == 0)
		{
			try
			{
				const wstring value = line.substr(7);
				size_t end = 0;
				const int32 room_id = stoi(value, &end);
				if (end != value.size() || room_id < 0 || room_id > 255) {
					throw out_of_range("room id");
				}
				if (!session->send_enter_room(static_cast<uint8>(room_id))) {
					wcout << L"이미 방에 있거나 방 요청의 응답을 기다리는 중입니다." << endl;
				}
			}
			catch (const exception&) {
				wcout << L"방 번호는 0~255의 정수로 입력하세요." << endl;
			}
		}
		else if (line == L"/exit")
		{
			if (!session->send_exit_room()) {
				wcout << L"입장한 방이 없거나 방 요청의 응답을 기다리는 중입니다." << endl;
			}
		}
		else if (line.rfind(L"/all ", 0) == 0) {
			session->send_chat(line.substr(5), Protocol::CHAT_STATE::CHAT_ALL, session->get_room_id());
		}
		else if (line.rfind(L"/chat ", 0) == 0) {
			session->send_chat(line.substr(6), Protocol::CHAT_STATE::CHAT_NORMAL, session->get_room_id());
		}
		else if (line.rfind(L"/whisper ", 0) == 0)
		{
			const wstring body = line.substr(9);
			const size_t split = body.find(L' ');
			if (split == wstring::npos)
			{
				wcout << L"usage: /whisper [name] [message]" << endl;
				continue;
			}
			session->send_whisper_chat(body.substr(0, split), body.substr(split + 1),
				Protocol::CHAT_STATE::CHAT_WHISPER, session->get_room_id());
		}
	}
	server.stop();
	wcout << L"클라이언트 종료 완료" << endl;
	return true;
}

void print_load_test_stats_loop(LoadControl& control)
{
	uint64 prev_send = 0;
	uint64 prev_recv = 0;
	auto prev_time = chrono::steady_clock::now();
	while (control.running.load())
	{
		const auto now = chrono::steady_clock::now();
		const double elapsed_sec = chrono::duration<double>(now - prev_time).count();
		prev_time = now;
		const uint64 sent = GServerStats.send_chat.load();
		const uint64 received = GServerStats.recv_chat.load();
		const uint64 expected = GServerStats.expected_chat_recv.load();
		const uint64 send_per_sec = elapsed_sec > 0 ? static_cast<uint64>((sent - prev_send) / elapsed_sec) : 0;
		const uint64 recv_per_sec = elapsed_sec > 0 ? static_cast<uint64>((received - prev_recv) / elapsed_sec) : 0;
		prev_send = sent;
		prev_recv = received;
		wcout << L"connect = " << GServerStats.connect_success.load()
			<< L" login = " << GServerStats.login_success.load()
			<< L" send = " << sent
			<< L" recv = " << received
			<< L" expected_recv = " << expected
			<< L" disconnect = " << GServerStats.disconnect.load()
			<< L" send/s = " << send_per_sec
			<< L" recv/s = " << recv_per_sec
			<< L" backlog = " << (expected > received ? expected - received : 0) << endl;
		control.wait(1000);
	}
}
