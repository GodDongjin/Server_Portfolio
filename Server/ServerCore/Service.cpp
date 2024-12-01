#include "pch.h"
#include "Service.h"
#include "Session.h"
#include "Listener.h"

Service::Service(ServiceType type, NetAddress address, IocpCoreRef core, SessionFactory factory, SessionManagerRef sessionManager)
	: mType(type), mNetAddress(address), mIocpCore(core), mSessionFactory(factory), mSessionManager(sessionManager)
{

}

Service::~Service()
{
}

bool Service::CanStart()
{
	if (mType == ServiceType::NONE || mIocpCore == nullptr || mSessionManager == nullptr)
		return false;

	return true;
}

void Service::CloseService()
{
	// TODO
}

SessionRef Service::CreateSession()
{
	SessionRef session = mSessionFactory();
	//session->SetService(shared_from_this());

	if (mIocpCore->Register(session) == false)
		return nullptr;

	return session;
}

ServerService::ServerService(NetAddress address, IocpCoreRef core, SessionFactory factory, SessionManagerRef sessionManager)
	: Service(ServiceType::Server, address, core, factory, sessionManager)
{
}

bool ServerService::Start()
{
	if (CanStart() == false)
		return false;

	_listener = make_shared<Listener>();
	if (_listener == nullptr)
		return false;

	ServerServiceRef service = static_pointer_cast<ServerService>(shared_from_this());
	if (_listener->StartAccept(service) == false)
		return false;

	return true;
}

void ServerService::CloseService()
{
	// TODO

	Service::CloseService();
}
