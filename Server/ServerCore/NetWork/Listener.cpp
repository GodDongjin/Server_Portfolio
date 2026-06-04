#include "pch.h"
#include "Listener.h"
#include "SocketUtils.h"
#include "Service.h"
#include "Session.h"
#include "SessionManager.h"

Listener::~Listener()
{
	SocketUtils::close(_socket);

	for (AcceptEvent* acceptEvent : _accept_events)
	{
		// TODO
		
		delete(acceptEvent);
	}
}

bool Listener::start_accept(ServerServiceRef service)
{
	_service = service;
	if (_service == nullptr)
		return false;

	_socket = SocketUtils::create_socket();
	if (_socket == INVALID_SOCKET)
		return false;

	if (_service->get_iocp_core()->Register(shared_from_this()) == false)
		return false;

	if (SocketUtils::set_resuse_address(_socket, true) == false)
		return false;

	if (SocketUtils::set_linger(_socket, 0, 0) == false)
		return false;

	if (SocketUtils::bind(_socket, _service->get_net_address()) == false)
		return false;

	if (SocketUtils::listen(_socket) == false)
		return false;

	const int32 acceptCount = _service->get_sessionManager()->get_max_session_count();
	for (int32 i = 0; i < acceptCount; i++)
	{
		AcceptEvent* acceptEvent = new AcceptEvent();
		acceptEvent->set_owner(shared_from_this());
		_accept_events.push_back(acceptEvent);
		register_accept(acceptEvent);
	}
	INFO_LOG("start accept");
	wcout << L"start accept" << endl;

	return true;
}

void Listener::close_socket()
{
	SocketUtils::close(_socket);
}

HANDLE Listener::get_handle()
{
	return reinterpret_cast<HANDLE>(_socket);
}

void Listener::dispatch(IocpEvent* iocpEvent, int32 numOfBytes)
{
	ASSERT_CRASH(iocpEvent->get_event_type() == EventType::Accept);
	AcceptEvent* acceptEvent = static_cast<AcceptEvent*>(iocpEvent);
	process_accept(acceptEvent);
}

void Listener::register_accept(AcceptEvent* acceptEvent)
{
	SessionRef session = _service->create_session();

	acceptEvent->init();
	acceptEvent->set_session(session);

	DWORD bytesReceived = 0;
	if (false == SocketUtils::AcceptEx(_socket, session->get_socket(), session->get_recv_buffer().write_pos(), 0, sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, OUT & bytesReceived, static_cast<LPOVERLAPPED>(acceptEvent)))
	{
		const int32 errorCode = ::WSAGetLastError();
		if (errorCode != WSA_IO_PENDING)
		{
			// 일단 다시 Accept 걸어준다
			register_accept(acceptEvent);
		}
	}
}

void Listener::process_accept(AcceptEvent* acceptEvent)
{
	SessionRef session = acceptEvent->get_session();

	if (false == SocketUtils::set_update_accept_socket(session->get_socket(), _socket))
	{
		register_accept(acceptEvent);
		return;
	}

	SOCKADDR_IN sockAddress;
	int32 sizeOfSockAddr = sizeof(sockAddress);
	if (SOCKET_ERROR == ::getpeername(session->get_socket(), OUT reinterpret_cast<SOCKADDR*>(&sockAddress), &sizeOfSockAddr))
	{
		register_accept(acceptEvent);
		return;
	}

	INFO_LOG("Success Accept");

	session->set_net_address(NetAddress(sockAddress));
	_service->get_sessionManager()->on_connected(session);
	register_accept(acceptEvent);
}