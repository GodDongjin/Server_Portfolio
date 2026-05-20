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
	if (_service_type == ServiceType::NONE || _iocp_core == nullptr || _sessionManager == nullptr)
		return false;

	return true;
}

void Service::close_service()
{
	// TODO
}

SessionRef Service::create_session()
{
	SessionRef session = _sessionFactory();
	//session->SetService(shared_from_this());

	if (_iocp_core->Register(session) == false)
		return nullptr;

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
		return false;

	return true;
}

void ServerService::close_service()
{
	// TODO

	Service::close_service();
}
