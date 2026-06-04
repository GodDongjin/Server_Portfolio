#include "TestSession.h"
#include "../Protocol/Protocol.pb.h"
#include "../Packet/ClientPacketHandler.h"
#include "../Utils/StringUtil.h"
#include "../Utils/GlobalStruct.h"

bool TestSession::start()
{
	if (is_connected() == false)
	{
		disconnect();
		cout << "connect false" << endl;
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
	if (_test_session_state == TEST_SESSION_STATE::CONNECTED) {
		cout << "test_session_state is CONNECTED" << endl;
		return false;
	}

	SOCKET client_socket = ::WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (client_socket == INVALID_SOCKET)
	{
		cout << "WSASocket failed : " << WSAGetLastError() << endl;
		WSACleanup();
		return false;
	}

	if (::connect(client_socket, reinterpret_cast<SOCKADDR*>(&server_addr), sizeof(server_addr)))
	{
		cout << "connect failed : " << WSAGetLastError() << endl;
		_is_connect = false;
		closesocket(client_socket);
		WSACleanup();
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
	if (_test_session_state == TEST_SESSION_STATE::DISCONNECTED) {
		cout << "test_session_state is DISCONNECTED" << endl;
		return;
	}

	if (_is_connect.exchange(false) == false)
	{
		return;
	}

	if (_socket == INVALID_SOCKET) {
		return;
	}

	_is_disconnect = true;
	GServerStats.disconnect++;

	::shutdown(_socket, SD_BOTH);
	::closesocket(_socket);
}

void TestSession::send(shared_ptr<SendBuffer> send_buffer)
{
	if (is_connected() == false)
		return;

	bool registerSend = false;

	{
		lock_guard<mutex> lock(_send_lock);

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
			return;
		}
	}
}

void TestSession::login()
{
	int is_create = 0;

	cout << "1. 로그인    2. 계정 생성   3. 나가기 " << endl;
	cin >> is_create;

	if (is_create == 3) {
		disconnect();
		return;
	}

	string id;
	string pw;
	wstring name;

	cout << "id 입력 : ";
	cin >> id;
	cout << "passward 입력 : ";
	cin >> pw;

	if (is_create == 2)
	{
		cout << "이름 입력 : ";
		wcin >> name;
	}

	Protocol::REQ_LOGIN _login_pkt;
	_login_pkt.set_is_create((is_create - 1));
	_login_pkt.set_id(id);
	_login_pkt.set_pw(pw);
	_login_pkt.set_name(WStringToUtf8(name));

	shared_ptr<SendBuffer> send_buffer = ClientPacketHandler::MakeSendBuffer(_login_pkt);
	send(send_buffer);
}

void TestSession::test_login(uint32 index, bool is_create_has_account)
{
	wstring name = L"Bot_" + to_wstring(index);
	string id = "bot_" + to_string(index);
	string pw = "1234";

	Protocol::REQ_BOT_LOGIN _login_pkt;
	_login_pkt.set_id(id);
	_login_pkt.set_pw(pw);
	_login_pkt.set_name(WStringToUtf8(name));
	
	if (!is_create_has_account) {
		_login_pkt.set_is_create(true);
	}
	else if (is_create_has_account) {
		_login_pkt.set_is_create(false);
	}
	

	shared_ptr<SendBuffer> send_buffer = ClientPacketHandler::MakeSendBuffer(_login_pkt);
	send(send_buffer);
}

void TestSession::logout()
{
	Protocol::REQ_LOGOUT _logout_pkt;
	_logout_pkt.set_idx(_account_idx);

	shared_ptr<SendBuffer> send_buffer = ClientPacketHandler::MakeSendBuffer(_logout_pkt);
	send(send_buffer);
}

void TestSession::send_chat(const wstring& message, Protocol::CHAT_STATE chat_state)
{
	Protocol::REQ_CHAT pkt;
	pkt.set_message(WStringToUtf8(message));
	pkt.set_chat_state(chat_state);

	auto sendBuffer = ClientPacketHandler::MakeSendBuffer(pkt);
	send(sendBuffer);

	GServerStats.send_chat++;
	GServerStats.expected_chat_recv += GServerStats.login_success.load();
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

}

bool TestSession::register_recv()
{
	if (is_connected() == false) {
		return false;
	}

	WSABUF wsa_buf;
	wsa_buf.buf = reinterpret_cast<char*>(_recv_buffer->write_pos());
	wsa_buf.len = _recv_buffer->free_size();

	DWORD numOfBytes = 0;
	DWORD flags = 0;
	if (SOCKET_ERROR == ::WSARecv(_socket, &wsa_buf, 1, OUT & numOfBytes, OUT & flags, &_recv_event, nullptr))
	{
		int32 errorCode = ::WSAGetLastError();
		if (errorCode != WSA_IO_PENDING)
		{
			cout << "Handle Error : " << errorCode << endl;
			_is_send_register.store(false);
			return false;
		}
	}

	return true;
}

bool TestSession::register_send()
{
	if (is_connected() == false) {
		return false;
	}

	_send_event.init();

	{
		lock_guard<mutex> lock(_send_lock);

		while (_send_queue.empty() == false)
		{
			shared_ptr<SendBuffer> sendBuffer = _send_queue.front();
			_send_queue.pop();
			_send_event.send_Buffers_push(sendBuffer);
		}
	}

	vector<WSABUF> wsaBufs;
	wsaBufs.reserve(_send_event.get_send_Buffers().size());
	for (const shared_ptr<SendBuffer>& sendBuffer : _send_event.get_send_Buffers())
	{
		WSABUF wsaBuf;
		wsaBuf.buf = reinterpret_cast<char*>(sendBuffer->get_buffer());
		wsaBuf.len = static_cast<LONG>(sendBuffer->get_write_size());
		wsaBufs.push_back(wsaBuf);
	}

	if (wsaBufs.empty())
	{
		_is_send_register.store(false);
		return true;
	}

	DWORD numOfBytes = 0;
	if (SOCKET_ERROR == ::WSASend(_socket, wsaBufs.data(), static_cast<DWORD>(wsaBufs.size()), OUT & numOfBytes, 0, &_send_event, nullptr))
	{
		int32 errorCode = ::WSAGetLastError();
		if (errorCode != WSA_IO_PENDING)
		{
			_send_event.send_Buffers_clear();
			_is_send_register.store(false);
		}
	}

	//cout << "register_send succes" << endl;

	return true;
}

void TestSession::process_send(uint32 num_bytes)
{
	_send_event.clear();

	if (num_bytes == 0)
	{
		disconnect();
		cout << "process_send : num_byte 0" << endl;
		return;
	}

	bool need_register = false;

	{
		lock_guard<mutex> lock(_send_lock);

		if (_send_queue.empty()){
			_is_send_register.store(false);
		}
		else{
			need_register = true;
		}
	}

	if (need_register)
	{
		register_send();
	}
}

void TestSession::process_recv(uint32 num_bytes)
{
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
			break;
		}

		process_len += header.size;
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


