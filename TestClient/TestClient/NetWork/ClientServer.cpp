#include "ClientServer.h"
#include "IocpEvent.h"

bool ClientServer::CreateIocpHandle()
{
	lock_guard<mutex> guard(_lock);
	if (_stopped.load()) {
		return false;
	}
	if (_iocp_handle != nullptr) {
		return true;
	}
	_iocp_handle = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);
	if (_iocp_handle == nullptr) {
		cout << "CreateIoCompletionPort failed : " << ::GetLastError() << endl;
		return false;
	}
	return true;
}

bool ClientServer::register_socket(shared_ptr<TestSession> session)
{
	if (session == nullptr) {
		return false;
	}
	lock_guard<mutex> guard(_lock);
	if (_stopped.load() || _iocp_handle == nullptr)
	{
		session->disconnect();
		return false;
	}
	for (const auto& registered : _sessions) {
		if (registered == session) {
			return true;
		}
	}
	if (!session->register_iocp(_iocp_handle))
	{
		session->disconnect();
		return false;
	}
	_sessions.push_back(session);
	return true;
}

bool ClientServer::start_server(const wstring& ip, uint16 port, SOCKADDR_IN& server_addr)
{
	server_addr = {};
	if (port == 0 || ::InetPtonW(AF_INET, ip.c_str(), &server_addr.sin_addr) != 1)
	{
		wcerr << L"Invalid server address: " << ip << L":" << port << endl;
		return false;
	}
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = ::htons(port);
	return true;
}

bool ClientServer::start_worker_thread(int32 thread_count)
{
	lock_guard<mutex> guard(_lock);
	if (_stopped.load() || _iocp_handle == nullptr || thread_count < 1 || thread_count > 256) {
		return false;
	}
	for (int32 i = 0; i < thread_count; i++) {
		_worker_threads.emplace_back([this]() { dispatch(); });
	}
	return true;
}

void ClientServer::stop()
{
	// 중복 종료 호출도 최초 종료 작업이 끝날 때까지 기다리게 한다.
	lock_guard<mutex> stop_guard(_stop_lock);
	vector<shared_ptr<TestSession>> sessions;
	{
		lock_guard<mutex> guard(_lock);
		if (_stopped.exchange(true)) {
			return;
		}
		sessions = _sessions;
	}

	for (const auto& session : sessions) {
		session->disconnect();
	}
	// 소켓 종료는 취소 요청일 뿐이다. 완료 처리가 끝나기 전에 버퍼를 해제하면 안 된다.
	if (_worker_threads.empty())
	{
		// 작업 스레드를 시작하기 전에 종료한 경우에도 현재 스레드가 완료를 회수한다.
		for (const auto& session : sessions) {
			while (session->has_pending_io()) {
				dispatch_once(100);
			}
		}
	}
	else
	{
		for (const auto& session : sessions) {
			session->wait_for_io();
		}
	}

	for (size_t i = 0; i < _worker_threads.size(); i++) {
		::PostQueuedCompletionStatus(_iocp_handle, 0, 0, nullptr);
	}
	for (thread& worker : _worker_threads) {
		if (worker.joinable()) {
			worker.join();
		}
	}
	_worker_threads.clear();
	_sessions.clear();
	if (_iocp_handle != nullptr)
	{
		::CloseHandle(_iocp_handle);
		_iocp_handle = nullptr;
	}
}

void ClientServer::dispatch()
{
	while (dispatch_once(INFINITE)) {}
}

bool ClientServer::dispatch_once(uint32 timeout_ms)
{
	DWORD num_bytes = 0;
	ULONG_PTR key = 0;
	IocpEvent* iocp_event = nullptr;
	BOOL result = ::GetQueuedCompletionStatus(_iocp_handle, &num_bytes, &key,
		reinterpret_cast<LPOVERLAPPED*>(&iocp_event), timeout_ms);
	if (iocp_event == nullptr)
	{
		if (result == FALSE && ::GetLastError() == WAIT_TIMEOUT) {
			return true;
		}
		return false;
	}

	TestSession* session = reinterpret_cast<TestSession*>(key);
	if (session == nullptr) {
		return true;
	}
	// 실패한 완료도 전달해야 I/O 개수와 송수신 버퍼가 정리된다.
	session->dispatch(iocp_event, result ? num_bytes : 0);
	return true;
}
