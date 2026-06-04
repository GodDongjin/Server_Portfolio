#include "ClientServer.h"
#include "IocpEvent.h"

bool ClientServer::CreateIocpHandle()
{
	_iocp_handle = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);
	
	if (_iocp_handle == nullptr) {
		cout << "CreateIocpHandle INVALID_HANDLE_VALUE" << GetLastError() << endl;
		return false;
	}

	return true;
}

bool ClientServer::register_socket(shared_ptr<TestSession> session)
{
	if (session == nullptr || *session->get_socket() == INVALID_SOCKET)
	{
		cout << "error : cloent_socket error" << endl;
		return false;
	}

	HANDLE result = CreateIoCompletionPort(reinterpret_cast<HANDLE>(*session->get_socket()), _iocp_handle, reinterpret_cast<ULONG_PTR>(session.get()), 0);

	if (result == nullptr)
	{
		cout << "error : " << GetLastError() << endl;

		cout << "register_scoket failed socket" <<  endl;
		closesocket(*session->get_socket());

		return false;
	}

	return true;
}

SOCKADDR_IN ClientServer::start_server(const wstring& ip, uint16 port)
{
	SOCKADDR_IN server_addr;
	::ZeroMemory(&server_addr, sizeof(server_addr));

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = ::htons(port);

	if (::InetPtonW(AF_INET, ip.c_str(), &server_addr.sin_addr) != 1)
	{
		//cout << "Invalid ip address : " << ip.c_str() << endl;
		WSACleanup();
		return server_addr;
	}

	return server_addr;
}

void ClientServer::start_worker_thread(short thread_count)
{
	lock_guard<mutex> guard(_lock);

	for (int i = 0; i < thread_count; i++)
	{
		_worker_threads.push_back(thread([=]()
		{
			dispatch();
		}));
	}
}

void ClientServer::stop()
{
	if (_stopped.exchange(true))
		return;

	for (size_t i = 0; i < _worker_threads.size(); i++)
	{
		::PostQueuedCompletionStatus(_iocp_handle, 0, 0, nullptr);
	}

	for (thread& t : _worker_threads)
	{
		if (t.joinable())
			t.join();
	}

	_worker_threads.clear();

	if (_iocp_handle)
	{
		::CloseHandle(_iocp_handle);
		_iocp_handle = nullptr;
	}
}

void ClientServer::dispatch()
{
	while (true)
	{
		DWORD numOfBytes = 0;
		ULONG_PTR key = 0;
		IocpEvent* iocp_event = nullptr;

		BOOL result = ::GetQueuedCompletionStatus(
			_iocp_handle,
			OUT & numOfBytes,
			OUT & key,
			OUT reinterpret_cast<LPOVERLAPPED*>(&iocp_event),
			INFINITE
		);

		if (iocp_event == nullptr)
			break;

		TestSession* session = reinterpret_cast<TestSession*>(key);
		if (session == nullptr)
			continue;

		if (result == FALSE)
		{
			session->disconnect();
			continue;
		}

		session->dispatch(iocp_event, numOfBytes);
	}
}
