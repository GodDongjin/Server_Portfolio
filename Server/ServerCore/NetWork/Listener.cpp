#include "pch.h"
#include "Listener.h"
#include "SocketUtils.h"
#include "Service.h"
#include "Session.h"
#include "SessionManager.h"
#include "../Job/JobQueue.h"

#define ACCEPT_COUNT 32

Listener::Listener() : _accept_retry_queue(make_shared<JobQueue>())
{
}

Listener::~Listener()
{
	SocketUtils::close(_socket);

	for (AcceptEvent* acceptEvent : _accept_events)
	{
		// close_service에서 취소된 Accept의 완료 처리까지 기다린 뒤 해제한다.
		delete(acceptEvent);
	}
}

bool Listener::start_accept(ServerServiceRef service)
{
	_service = service;
	if (_service == nullptr) {
		return false;
	}

	_socket = SocketUtils::create_socket();
	if (_socket == INVALID_SOCKET) {
		return false;
	}
		
	if(_service->get_iocp_core() == nullptr) {
		return false;
	}

	if (_service->get_iocp_core()->Register(shared_from_this()) == false) {
		return false;
	}

	if (SocketUtils::set_resuse_address(_socket, true) == false) {
		return false;
	}

	if (SocketUtils::set_linger(_socket, 0, 0) == false) {
		return false;
	}

	if (SocketUtils::bind(_socket, _service->get_net_address()) == false) {
		return false;
	}

	if (SocketUtils::listen(_socket) == false) {
		return false;
	}

	//const int32 acceptCount = _service->get_sessionManager()->get_max_session_count();
	for (int32 i = 0; i < ACCEPT_COUNT; i++)
	{
		AcceptEvent* acceptEvent = new AcceptEvent();
		_accept_events.push_back(acceptEvent);
		register_accept(acceptEvent);
	}
	INFO_LOG("start accept");
	wcout << L"start accept" << endl;

	return true;
}

void Listener::close_socket()
{
	WRITE_LOCK;

	// 예약된 작업이 나중에 실행돼도 Accept를 다시 등록하지 않는다.
	if (_accept_stopped.exchange(true))
		return;
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
	WRITE_LOCK;

	if (_accept_stopped || _socket == INVALID_SOCKET) {
		return;
	}

	SessionRef session = _service->create_session();
	if (session == nullptr || session->get_socket() == INVALID_SOCKET)
	{
		ERROR_LOG("Accept session creation failed");
		acceptEvent->set_session(nullptr);
		acceptEvent->set_owner(nullptr);
		reserve_accept_retry(acceptEvent);
		return;
	}

	acceptEvent->init();
	acceptEvent->set_session(session);
	// 진행 중인 Accept의 완료 통지를 받을 때까지 리스너와 세션을 유지한다.
	acceptEvent->set_owner(shared_from_this());

	begin_io();

	DWORD bytesReceived = 0;
	if (SocketUtils::AcceptEx(_socket, session->get_socket(), session->get_recv_buffer().write_pos(), 0, sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, OUT & bytesReceived, static_cast<LPOVERLAPPED>(acceptEvent))) {
		return;
	}

	const int32 errorCode = ::WSAGetLastError();
	// 정상적으로 비동기 대기 중이므로 중복 등록하지 않는다.
	if (errorCode == WSA_IO_PENDING) {
		return;
	}

	// 즉시 실패하면 완료 통지가 오지 않으므로 참조를 여기서 해제한다.
	acceptEvent->set_session(nullptr);
	acceptEvent->set_owner(nullptr);

	finish_io();

	ERROR_LOG("AcceptEx failed: " + to_string(errorCode));

	switch (errorCode)
	{
	case WSAENOBUFS:
	case WSAEMFILE:
	case WSAECONNRESET:
	case WSAECONNABORTED:
		reserve_accept_retry(acceptEvent);
		break;
	default:
		// 재시도 대상으로 정하지 않은 오류는 해당 슬롯의 등록을 중단한다.
		ERROR_LOG("Accept retry stopped: " + to_string(errorCode));
		break;
	}
}

void Listener::reserve_accept_retry(AcceptEvent* acceptEvent)
{
	// 진행 중인 Accept가 없는 슬롯에서만 예약한다.
	// 예약 자체가 리스너 수명을 늘리지 않도록 약한 참조를 캡처한다.
	weak_ptr<Listener> weak_listener = static_pointer_cast<Listener>(shared_from_this());
	_accept_retry_queue->do_timer(ACCEPT_RETRY_INTERVAL_MS, [weak_listener, acceptEvent]()
	{
		auto listener = weak_listener.lock();
		if (listener == nullptr) {
			return;
		}

		// 리스너가 살아 있는 동안 해당 리스너 소유의 AcceptEvent도 유지된다.
		listener->register_accept(acceptEvent);
	});
}

void Listener::process_accept(AcceptEvent* acceptEvent)
{
	SessionRef session;
	SOCKADDR_IN sockAddress;
	{
		WRITE_LOCK;

		// 완료된 이벤트의 참조를 해제한다. IOCP가 현재 호출의 리스너 수명을 유지한다.
		session = acceptEvent->get_session();
		acceptEvent->set_session(nullptr);
		acceptEvent->set_owner(nullptr);

		if (_accept_stopped || _socket == INVALID_SOCKET || session == nullptr) {
			return;
		}

		// 소켓 종료와 겹치지 않도록 완료 처리에서도 같은 잠금을 사용한다.
		if (false == SocketUtils::set_update_accept_socket(session->get_socket(), _socket))
		{
			reserve_accept_retry(acceptEvent);
			return;
		}

		int32 sizeOfSockAddr = sizeof(sockAddress);
		if (SOCKET_ERROR == ::getpeername(session->get_socket(), OUT reinterpret_cast<SOCKADDR*>(&sockAddress), &sizeOfSockAddr))
		{
			reserve_accept_retry(acceptEvent);
			return;
		}
	}

	INFO_LOG("Success Accept");
	session->set_net_address(NetAddress(sockAddress));
	if (!_service->get_sessionManager()->on_connected(session))
	{
		// 초과 연결은 정상 세션에 등록하지 않고, 안내 송신이 끝난 뒤 종료한다.
		session->reject_connection(_service->get_sessionManager()->get_max_session_count());
	}
	// register_accept에서 같은 잠금을 사용하므로 위 잠금을 해제한 뒤 호출한다.
	register_accept(acceptEvent);
}
