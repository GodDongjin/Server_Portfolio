#pragma once
#include "Session.h"

class SessionManager
{
public:
	SessionManager(int32 sessionMaxCount);

public:
	void add_session(SessionRef session);
	void release_session(SessionRef session);
	bool broad_cast(SendBufferRef sendBuffer);

public:
	void on_connected(SessionRef session);
	void on_disconnected(SessionRef session);

public:
	int32 get_max_session_count() { return _max_session_count; }

private:
	USE_LOCK;

	set<SessionRef> _sessions;
	int32 _sessionCount;
	int32 _max_session_count;

	atomic<uint64_t> _session_id{ 1 };

};

