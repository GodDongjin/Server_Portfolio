#include "ClientServer.h"
#include "IocpEvent.h"
#include "../Session/TestSession.h"

bool ClientServer::CreateIocpHandle()
{
	_iocp_handle = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);
	
	if (_iocp_handle != INVALID_HANDLE_VALUE) {
		return false;
	}

	return true;
}

bool ClientServer::register_socket(SOCKET* client_socket)
{
	if (client_socket == nullptr || *client_socket == INVALID_SOCKET)
	{
		cout << "error : cloent_socket error" << endl;
		return false;
	}

	HANDLE result = CreateIoCompletionPort(reinterpret_cast<HANDLE>(*client_socket), _iocp_handle, 0, 0);

	if (result == nullptr)
	{
		cout << "error : " << GetLastError() << endl;

		cout << "register_scoket failed socket" <<  endl;
		closesocket(*client_socket);

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
		_worker_trheads.push_back(thread([=]()
		{
			dispatch();
		}));
	}
}

void ClientServer::dispatch()
{
	while (true)
	{
		DWORD numOfBytes = 0;
		ULONG_PTR key = 0;
		IocpEvent* iocp_event = nullptr;

		if (::GetQueuedCompletionStatus(_iocp_handle, OUT & numOfBytes, OUT & key, OUT reinterpret_cast<LPOVERLAPPED*>(&iocp_event), INFINITE))
		{
			shared_ptr<TestSession> session = iocp_event->get_session();
			//session->dispatch(iocp_event);
		}
	}
}
