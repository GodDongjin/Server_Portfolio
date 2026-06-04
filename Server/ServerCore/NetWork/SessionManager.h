#pragma once
#include "Session.h"

class SessionManager
{
public:
	SessionManager(int32 sessionMaxCount);

public:
	void add_session(SessionRef session);
	void release_session(SessionRef session);
	int32 broad_cast(SendBufferRef sendBuffer);

public:
	void on_connected(SessionRef session);
	void on_disconnected(SessionRef session);

public:
	int32 get_max_session_count() { return _max_session_count; }

private:
	USE_LOCK;

	set<SessionRef> _sessions;
	int32 _session_count;
	int32 _max_session_count;
};

