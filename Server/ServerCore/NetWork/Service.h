#pragma once
#include "NetAddress.h"
#include "IocpCore.h"
#include "Listener.h"
#include "SessionManager.h"
#include <functional>

enum class ServiceType : uint8
{
	NONE,
	SERVER,
	CLIENT
};

using SessionFactory = function<SessionRef(void)>;

class Service : public enable_shared_from_this<Service>
{
public:
	Service(ServiceType type, NetAddress address, IocpCoreRef core, SessionFactory factory, SessionManagerRef sessionManager);
	virtual ~Service();

	virtual bool		start() abstract;
	bool				can_start();

	virtual void		close_service();
	SessionRef			create_session();
	void				release_live_session(Session* session);

	void				set_sessionFactory(SessionFactory func) { _sessionFactory = func; }

public:
	ServiceType			get_service_type() { return _service_type; }
	NetAddress			get_net_address() { return _net_address; }
	IocpCoreRef&		get_iocp_core() { return _iocp_core; }
	SessionManagerRef&  get_sessionManager() { return _sessionManager; }

private:
	// 정원 목록과 별도로 Accept 대기 및 정원 초과 안내 세션의 수명도 추적한다.
	mutex _sessions_lock;
	map<Session*, weak_ptr<Session>> _live_sessions;
	bool _closing = false;

protected:
	USE_LOCK;
	ServiceType			_service_type = ServiceType::NONE;
	NetAddress			_net_address = {};
	IocpCoreRef			_iocp_core = nullptr;
	SessionFactory		_sessionFactory;
	SessionManagerRef	_sessionManager = nullptr;
};

/*-----------------
	ServerService
------------------*/

class ServerService : public Service
{
public:
	ServerService(NetAddress targetAddress, IocpCoreRef core, SessionFactory factory, SessionManagerRef sessionManager);
	virtual ~ServerService() {}

	virtual bool	start() override;
	virtual void	close_service() override;

private:
	mutex _close_lock;
	ListenerRef		_listener = nullptr;
};