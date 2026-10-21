#include "pch.h"
#include "Service.h"
#include "Session.h"
#include "Listener.h"

Service::Service(ServiceType type, NetAddress address, IocpCoreRef core, SessionFactory factory, SessionManagerRef sessionManager)
	: _service_type(type), _net_address(address), _iocp_core(core), _sessionFactory(factory), _sessionManager(sessionManager)
{

}

Service::~Service()
{
}

bool Service::can_start()
{
	if (_service_type == ServiceType::NONE || _iocp_core == nullptr || _sessionManager == nullptr) {
		return false;
	}

	return true;
}

void Service::close_service()
{
	vector<SessionRef> sessions;
	{
		lock_guard<mutex> guard(_sessions_lock);
		_closing = true;
		for (auto& [address, weak_session] : _live_sessions)
		{
			if (auto session = weak_session.lock())
				sessions.push_back(std::move(session));
		}
	}

	// 전체 세션에 먼저 종료를 요청해야 특정 클라이언트 때문에 다른 종료가 지연되지 않는다.
	for (SessionRef& session : sessions)
		session->begin_shutdown();
	for (SessionRef& session : sessions)
		session->wait_for_shutdown();
}

void Service::release_live_session(Session* session)
{
	lock_guard<mutex> guard(_sessions_lock);
	_live_sessions.erase(session);
}

SessionRef Service::create_session()
{
	// 지역 세션은 잠금 해제 후 소멸하게 해 소멸자의 추적 해제와 충돌하지 않게 한다.
	SessionRef session;
	lock_guard<mutex> guard(_sessions_lock);
	if (_closing)
		return nullptr;
	session = _sessionFactory();
	// 생성 실패를 리스너에 전달해 지연 재시도 경로에서 처리한다.
	if (session == nullptr || session->get_socket() == INVALID_SOCKET) {
		return nullptr;
	}

	if (_iocp_core->Register(session) == false) {
		return nullptr;
	}

	session->set_service(shared_from_this());
	_live_sessions[session.get()] = session;
	return session;
}

ServerService::ServerService(NetAddress address, IocpCoreRef core, SessionFactory factory, SessionManagerRef sessionManager)
	: Service(ServiceType::SERVER, address, core, factory, sessionManager)
{
}

bool ServerService::start()
{
	if (can_start() == false)
		return false;

	_listener = make_shared<Listener>();
	if (_listener == nullptr)
		return false;

	ServerServiceRef service = static_pointer_cast<ServerService>(shared_from_this());
	if (_listener->start_accept(service) == false)
	{
		// Accept 등록 전 시작 실패에서도 상호 참조와 리슨 소켓을 해제한다.
		_listener->close_socket();
		_listener.reset();
		return false;
	}

	return true;
}

void ServerService::close_service()
{
	// 종료는 IOCP 작업 스레드 밖에서 호출한다. 중복 호출도 이전 종료 완료를 기다린다.
	lock_guard<mutex> guard(_close_lock);
	if (_listener)
	{
		_listener->close_socket();
		_listener->wait_for_io();
	}

	Service::close_service();
	_listener.reset();
}
