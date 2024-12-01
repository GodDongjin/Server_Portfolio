#pragma once
#include "NetAddress.h"
#include "IocpCore.h"
#include "Listener.h"
#include "SessionManager.h"
#include <functional>

enum class ServiceType : uint8
{
	NONE,
	Server,
	Client
};

using SessionFactory = function<SessionRef(void)>;

class Service : public enable_shared_from_this<Service>
{
public:
	Service(ServiceType type, NetAddress address, IocpCoreRef core, SessionFactory factory, SessionManagerRef sessionManager);
	virtual ~Service();

	virtual bool		Start() abstract;
	bool				CanStart();

	virtual void		CloseService();
	SessionRef			CreateSession();

	void				SetSessionFactory(SessionFactory func) { mSessionFactory = func; }

	//int32				GetCurrentSessionCount() { return mSessionManager; }
	//int32				GetMaxSessionCount() { return mMaxSessionCount; }

public:
	ServiceType			GetServiceType() { return mType; }
	NetAddress			GetNetAddress() { return mNetAddress; }
	IocpCoreRef&		GetIocpCore() { return mIocpCore; }
	SessionManagerRef&  GetSessionManager() { return mSessionManager; }

protected:
	USE_LOCK;
	ServiceType			mType = ServiceType::NONE;
	NetAddress			mNetAddress = {};
	IocpCoreRef			mIocpCore = nullptr;
	SessionFactory		mSessionFactory;
	SessionManagerRef	mSessionManager = nullptr;
};

/*-----------------
	ServerService
------------------*/

class ServerService : public Service
{
public:
	ServerService(NetAddress targetAddress, IocpCoreRef core, SessionFactory factory, SessionManagerRef sessionManager);
	virtual ~ServerService() {}

	virtual bool	Start() override;
	virtual void	CloseService() override;

private:
	ListenerRef		_listener = nullptr;
};