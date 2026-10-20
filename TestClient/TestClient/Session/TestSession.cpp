#include "TestSession.h"
#include "../Protocol/Protocol.pb.h"
#include "../Packet/ClientPacketHandler.h"
#include "../Utils/StringUtil.h"
#include "../Utils/GlobalData.h"

bool TestSession::start()
{
	if (is_connected() == false)
	{
		disconnect();
		cout << "connect false" << endl;
		return false;
	}

	if (_recv_started.exchange(true)) {
		return false;
	}

	if (!register_recv()) {
		disconnect();
		cout << "register_recv failed" << endl;
		return false;
	}

	return true;
}

bool TestSession::connect(SOCKADDR_IN server_addr)
{
	lock_guard<mutex> lock(_send_lock);
	if (is_connected() || _is_disconnect.load()) {
		cout << "test_session_state is CONNECTED" << endl;
		return false;
	}

	SOCKET client_socket = ::WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (client_socket == INVALID_SOCKET)
	{
		cout << "WSASocket failed : " << WSAGetLastError() << endl;
		return false;
	}

	if (::connect(client_socket, reinterpret_cast<SOCKADDR*>(&server_addr), sizeof(server_addr)))
	{
		cout << "connect failed : " << WSAGetLastError() << endl;
		_is_connect = false;
		closesocket(client_socket);
		return false;
	}

	_socket = client_socket;
	_is_connect.store(true);
	_test_session_state = TEST_SESSION_STATE::CONNECTED;

	GServerStats.connect_success++;

	return true;
}

void TestSession::disconnect()
{
	// 송신 등록과 소켓 종료가 겹치지 않도록 같은 잠금을 사용한다.
	lock_guard<mutex> lock(_send_lock);

	if (_is_connect.exchange(false) == false){
		return;
	}

	if (_is_disconnect.exchange(true) == true) {
		return;
	}

	_test_session_state = TEST_SESSION_STATE::DISCONNECTED;
	_is_login.store(false);
	_account_idx.store(0);
	_room_id.store(-1);
	_login_pending = false;
	_room_request = ROOM_REQUEST::NONE;
	_state_changed.notify_all();
	_is_send_register.store(false);

	// 아직 등록하지 않은 데이터는 버린다. 진행 중인 버퍼는 완료 통지까지 유지한다.
	queue<shared_ptr<SendBuffer>> empty_queue;
	_send_queue.swap(empty_queue);

	GServerStats.disconnect++;

	if (_socket != INVALID_SOCKET)
	{
		::shutdown(_socket, SD_BOTH);
		::closesocket(_socket);
		_socket = INVALID_SOCKET;
	}
}

bool TestSession::send(shared_ptr<SendBuffer> send_buffer)
{
	if (send_buffer == nullptr || send_buffer->get_write_size() <= 0) {
		return false;
	}

	bool registerSend = false;

	{
		lock_guard<mutex> lock(_send_lock);

		// 종료와 경합한 송신 요청이 큐에 남거나 새 송신을 시작하지 않도록 안에서 확인한다.
		if (is_connected() == false) {
			return false;
		}

		_send_queue.push(send_buffer);

		if (_is_send_register.exchange(true) == false) {
			registerSend = true;
		}
	}

	if (registerSend) {

		if (!register_send())
		{
			// Log 찍어야함.
			wcout << L"register_send Fail" << endl;
			return false;
		}
	}
	return true;
}

TestSession::~TestSession()
{
	// ClientServer가 완료를 모두 회수한 뒤 소유권을 해제한다.
	assert(!has_pending_io());
	if (_socket != INVALID_SOCKET) {
		::closesocket(_socket);
	}
}

bool TestSession::register_iocp(HANDLE iocp_handle)
{
	lock_guard<mutex> lock(_send_lock);
	if (!is_connected() || _socket == INVALID_SOCKET) {
		return false;
	}
	if (::CreateIoCompletionPort(reinterpret_cast<HANDLE>(_socket), iocp_handle,
		reinterpret_cast<ULONG_PTR>(this), 0) == nullptr)
	{
		cout << "register socket failed : " << ::GetLastError() << endl;
		return false;
	}
	return true;
}

void TestSession::begin_io()
{
	lock_guard<mutex> lock(_io_lock);
	++_pending_io;
}

void TestSession::finish_io()
{
	lock_guard<mutex> lock(_io_lock);
	assert(_pending_io > 0);
	if (--_pending_io == 0) {
		_io_completed.notify_all();
	}
}

bool TestSession::has_pending_io()
{
	lock_guard<mutex> lock(_io_lock);
	return _pending_io != 0;
}

void TestSession::wait_for_io()
{
	unique_lock<mutex> lock(_io_lock);
	_io_completed.wait(lock, [this]() { return _pending_io == 0; });
}

bool TestSession::begin_login()
{
	lock_guard<mutex> lock(_send_lock);
	if (!is_connected() || get_is_login() || _login_pending) {
		return false;
	}
	_login_pending = true;
	_test_session_state.store(TEST_SESSION_STATE::LOGIN_SEND);
	return true;
}

bool TestSession::login(bool is_create, const string& id, const string& pw, const wstring& name)
{
	const string user_name = WStringToUtf8(name);
	if (id.empty() || id.size() > 32 || pw.empty() || pw.size() > 32 ||
		(is_create && (user_name.empty() || user_name.size() > 32)))
	{
		wcout << L"아이디·비밀번호·닉네임은 UTF-8 기준 1~32바이트로 입력하세요." << endl;
		return false;
	}
	if (!begin_login()) {
		return false;
	}
	Protocol::REQ_LOGIN pkt;
	pkt.set_is_create(is_create);
	pkt.set_id(id);
	pkt.set_pw(pw);
	pkt.set_name(user_name);
	if (!send(ClientPacketHandler::MakeSendBuffer(pkt)))
	{
		complete_login(false);
		return false;
	}
	return true;
}

bool TestSession::test_login(uint32 index, bool is_create_has_account)
{
	if (!begin_login()) {
		return false;
	}
	Protocol::REQ_BOT_LOGIN pkt;
	pkt.set_id("bot_" + to_string(index));
	pkt.set_pw("1234");
	pkt.set_name("Bot_" + to_string(index));
	pkt.set_is_create(!is_create_has_account);
	if (!send(ClientPacketHandler::MakeSendBuffer(pkt)))
	{
		complete_login(false);
		return false;
	}
	return true;
}

void TestSession::complete_login(bool success, uint64 account_idx)
{
	lock_guard<mutex> lock(_send_lock);
	if (is_connected())
	{
		_account_idx.store(success ? account_idx : 0);
		_is_login.store(success);
		_test_session_state.store(success ? TEST_SESSION_STATE::LOGIN : TEST_SESSION_STATE::CONNECTED);
	}
	_login_pending = false;
	_state_changed.notify_all();
}

bool TestSession::wait_for_login(uint32 timeout_ms)
{
	unique_lock<mutex> lock(_send_lock);
	return _state_changed.wait_for(lock, chrono::milliseconds(timeout_ms),
		[this]() { return !_login_pending || !is_connected(); });
}

bool TestSession::wait_for_disconnect(uint32 timeout_ms)
{
	unique_lock<mutex> lock(_send_lock);
	return _state_changed.wait_for(lock, chrono::milliseconds(timeout_ms),
		[this]() { return !is_connected(); });
}

void TestSession::logout()
{
	Protocol::REQ_LOGOUT pkt;
	pkt.set_idx(get_account());
	send(ClientPacketHandler::MakeSendBuffer(pkt));
}

void TestSession::send_get_room_info()
{
	Protocol::REQ_GET_ROOM_INFO pkt;
	send(ClientPacketHandler::MakeSendBuffer(pkt));
}

bool TestSession::send_enter_room(uint8 room_id)
{
	{
		lock_guard<mutex> lock(_send_lock);
		if (!is_connected() || !get_is_login() || get_room_id() >= 0 ||
			_room_request != ROOM_REQUEST::NONE) {
			return false;
		}
		_room_request = ROOM_REQUEST::ENTER;
	}
	Protocol::REQ_ENTER_ROOM pkt;
	pkt.set_room_id(room_id);
	if (!send(ClientPacketHandler::MakeSendBuffer(pkt)))
	{
		complete_enter_room(false, -1);
		return false;
	}
	return true;
}

bool TestSession::send_exit_room()
{
	{
		lock_guard<mutex> lock(_send_lock);
		if (!is_connected() || !get_is_login() || get_room_id() < 0 ||
			_room_request != ROOM_REQUEST::NONE) {
			return false;
		}
		_room_request = ROOM_REQUEST::EXIT;
	}
	Protocol::REQ_EXIT_ROOM pkt;
	if (!send(ClientPacketHandler::MakeSendBuffer(pkt)))
	{
		complete_exit_room(false);
		return false;
	}
	return true;
}

void TestSession::complete_enter_room(bool success, int32 room_id)
{
	lock_guard<mutex> lock(_send_lock);
	if (_room_request != ROOM_REQUEST::ENTER) {
		return;
	}
	// 실패 응답으로 이미 확정된 방 정보를 지우지 않는다.
	if (is_connected() && success) {
		_room_id.store(room_id);
	}
	_room_request = ROOM_REQUEST::NONE;
}

void TestSession::complete_exit_room(bool success)
{
	lock_guard<mutex> lock(_send_lock);
	if (_room_request != ROOM_REQUEST::EXIT) {
		return;
	}
	if (is_connected() && success) {
		_room_id.store(-1);
	}
	_room_request = ROOM_REQUEST::NONE;
}

void TestSession::send_chat(const wstring& message, Protocol::CHAT_STATE chat_state, int32 room_id)
{
	const string text = WStringToUtf8(message);
	if (text.empty() || text.size() > 256)
	{
		wcout << L"채팅은 UTF-8 기준 1~256바이트로 입력하세요." << endl;
		return;
	}
	Protocol::REQ_CHAT pkt;
	pkt.set_message(text);
	pkt.set_chat_state(chat_state);
	pkt.set_room_id(room_id);
	if (send(ClientPacketHandler::MakeSendBuffer(pkt))) {
		GServerStats.send_chat++;
	}
	// 예상 수신량은 ACK_CHAT에 담긴 서버의 전송 대상 수로 집계한다.
}

void TestSession::send_whisper_chat(const wstring& target_name, const wstring& message,
	Protocol::CHAT_STATE chat_state, int32 room_id)
{
	const string target = WStringToUtf8(target_name);
	const string text = WStringToUtf8(message);
	if (target.empty() || target.size() > 32 || text.empty() || text.size() > 256)
	{
		wcout << L"귓속말 대상은 1~32바이트, 채팅은 1~256바이트로 입력하세요." << endl;
		return;
	}
	Protocol::REQ_CHAT pkt;
	pkt.set_message(text);
	pkt.set_chat_state(chat_state);
	pkt.set_target_name(target);
	if (send(ClientPacketHandler::MakeSendBuffer(pkt))) {
		GServerStats.send_chat++;
	}
	// 예상 수신량은 ACK_CHAT에 담긴 서버의 전송 대상 수로 집계한다.
}


void TestSession::dispatch(IocpEvent* iocp_evnet, INT32 numOfbyte)
{
	switch (iocp_evnet->get_event_type())
	{
	case EventType::SEND:
		process_send(numOfbyte);
		break;
	case EventType::RECV:
		process_recv(numOfbyte);
		break;

	default:
		break;
	}

	// 콜백에서 다음 I/O를 등록한 뒤 현재 완료를 차감한다.
	finish_io();
}

bool TestSession::register_recv()
{
	int32 errorCode = 0;
	{
		// 소켓을 사용하는 동안 disconnect가 핸들을 닫지 않도록 보호한다.
		lock_guard<mutex> lock(_send_lock);
		if (is_connected() == false) {
			return false;
		}

		_recv_event.init();
		WSABUF wsa_buf;
		wsa_buf.buf = reinterpret_cast<char*>(_recv_buffer->write_pos());
		wsa_buf.len = _recv_buffer->free_size();

		DWORD numOfBytes = 0;
		DWORD flags = 0;
		begin_io();
		if (SOCKET_ERROR == ::WSARecv(_socket, &wsa_buf, 1, OUT & numOfBytes, OUT & flags, &_recv_event, nullptr)) {
			errorCode = ::WSAGetLastError();
			if (errorCode != WSA_IO_PENDING) {
				finish_io();
			}
		}
	}

	if (errorCode != 0 && errorCode != WSA_IO_PENDING)
	{
		// 수신 실패가 진행 중인 송신 플래그를 바꾸지 않도록 연결 종료로 처리한다.
		cout << "WSARecv failed : " << errorCode << endl;
		disconnect();
		return false;
	}

	return true;
}

bool TestSession::register_send()
{
	int32 errorCode = 0;
	{
		lock_guard<mutex> lock(_send_lock);
		if (is_connected() == false)
		{
			// 최초 송신 또는 이전 완료 이후의 재등록만 이 함수에 진입한다.
			_send_event.clear();
			_is_send_register.store(false);
			return false;
		}

		_send_event.init();

		// 부분 송신 중에는 새 데이터를 섞지 않고 기존 버퍼의 남은 구간부터 보낸다.
		if (!_send_event.has_pending_data())
		{
			while (!_send_queue.empty())
			{
				_send_event.send_Buffers_push(_send_queue.front());
				_send_queue.pop();
			}
		}

		vector<WSABUF> wsaBufs;
		_send_event.build_wsabufs(wsaBufs);
		if (wsaBufs.empty())
		{
			_send_event.clear();
			_is_send_register.store(false);
			return true;
		}

		begin_io();
		if (SOCKET_ERROR == ::WSASend(_socket, wsaBufs.data(), static_cast<DWORD>(wsaBufs.size()), nullptr, 0, &_send_event, nullptr))
		{
			errorCode = ::WSAGetLastError();
			if (errorCode != WSA_IO_PENDING)
			{
				finish_io();
				// 즉시 실패하면 완료 통지가 오지 않으므로 버퍼 참조를 여기서 해제한다.
				_send_event.clear();
				// 종료 전까지 송신 중 플래그를 유지해 다른 요청의 새 등록을 막는다.
			}
		}
	}

	if (errorCode != 0 && errorCode != WSA_IO_PENDING)
	{
		cout << "WSASend failed : " << errorCode << endl;
		// disconnect도 같은 잠금을 사용하므로 잠금을 해제한 뒤 호출한다.
		disconnect();
		return false;
	}

	return true;
}

void TestSession::process_send(uint32 num_bytes)
{
	bool need_register = false;
	bool need_disconnect = false;
	{
		lock_guard<mutex> lock(_send_lock);
		if (is_connected() == false)
		{
			// 취소되거나 종료된 송신도 완료 통지를 받은 뒤 버퍼를 해제한다.
			_send_event.clear();
			_is_send_register.store(false);
			return;
		}

		if (!_send_event.consume_sent_bytes(num_bytes))
		{
			_send_event.clear();
			need_disconnect = true;
		}
		else if (_send_event.has_pending_data())
		{
			// 송신 완료 바이트 수만큼 이동한 위치에서 이어 보낸다.
			need_register = true;
		}
		else
		{
			// 현재 묶음의 전체 송신이 끝난 뒤에만 다음 큐 데이터를 처리한다.
			_send_event.clear();
			if (_send_queue.empty()) {
				_is_send_register.store(false);
			}
			else {
				need_register = true;
			}
		}
	}

	if (need_disconnect)
	{
		cout << "process_send : invalid completion size" << endl;
		disconnect();
		return;
	}

	if (need_register) {
		register_send();
	}
}

void TestSession::process_recv(uint32 num_bytes)
{
	if (is_connected() == false) {
		return;
	}

	if (num_bytes == 0) {
		disconnect();
		cout << "process_recv - num_bytes is 0" << endl;
		return;
	}
	
	if (_recv_buffer->on_write(num_bytes) == false)
	{
		disconnect();
		cout << "process_recv - on_write Overflow" << endl;
		return;
	}

	INT32 process_len = 0;
	INT32 recv_len = _recv_buffer->data_size();
	BYTE* recv_buffer = _recv_buffer->read_pos();
	INT32 data_size = 0;
	while (true)
	{
		data_size = recv_len - process_len;

		if (data_size < sizeof(PacketHeader)) {
			break;
		}

		PacketHeader header = *(reinterpret_cast<PacketHeader*>(&recv_buffer[process_len]));

		if (header.size < sizeof(PacketHeader))
		{
			disconnect();
			return;
		}

		if (header.size > MAX_PACKET_SIZE)
		{
			disconnect();
			return;
		}

		// 패킷 사이즈 채크.
		if (data_size < header.size) {
			cout << "data_size가 패킷 사이즈 보다 작음 " << endl;
			break;
		}

		// 패킷 처리 작업 진행.
		auto session = shared_from_this();
		if (!ClientPacketHandler::HandlePacket(session, &recv_buffer[process_len], header.size))
		{
			cout << "HandlePacket ERROR - ID : " << header.id << endl;
			disconnect();
			return;
		}

		process_len += header.size;

		// 정원 초과 안내 등으로 종료한 경우 남아 있는 패킷을 더 처리하지 않는다.
		if (!is_connected())
			break;
	}


	if (process_len < 0 || process_len > recv_len || _recv_buffer->on_read(process_len) == false)
	{
		disconnect();
		cout << "on_read Overflow" << endl;
		return;
	}

	_recv_buffer->clean();

	register_recv();
}
