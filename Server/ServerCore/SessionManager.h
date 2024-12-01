#pragma once
#include "Session.h"

class SessionManager
{
public:
	SessionManager(int32 sessionMaxCount);
public:
	void AddSession(SessionRef session);
	void ReleaseSession(SessionRef session);
	bool Broadcast(SendBufferRef sendBuffer);

public:
	void OnConnected(SessionRef session);
	void OnDisconnected(SessionRef session);

public:
	int32 GetMaxSessionCount() { return mMaxSessionCount; }

private:
	USE_LOCK;

	set<SessionRef> mSessions;
	int32 mSessionCount;
	int32 mMaxSessionCount;

};

