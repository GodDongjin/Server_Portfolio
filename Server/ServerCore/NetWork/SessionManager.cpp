#include "pch.h"
#include "SessionManager.h"

SessionManager::SessionManager(int32 sessionMaxCount)
	: _max_session_count(sessionMaxCount), _session_count(0)
{
}

void SessionManager::add_session(SessionRef session)
{
	WRITE_LOCK;

	if (_session_count >= _max_session_count)
	{
		session->disconnect();
		return;
	}

	_sessions.insert(session);
	session->set_process_connect();
	_session_count++;
}

void SessionManager::release_session(SessionRef session)
{
	WRITE_LOCK;
	if (_sessions.erase(session) == 0)
		return;

	_session_count--;
}

int32 SessionManager::broad_cast(SendBufferRef sendBuffer)
{
	vector<SessionRef> sessions;

	{
		WRITE_LOCK;
		sessions.reserve(_session_count);
		sessions.assign(_sessions.begin(), _sessions.end());
	}

	int32 count = 0;

	for (SessionRef& session : sessions)
	{
		if (session->is_connected() == false)
			continue;

		session->send(sendBuffer);
		count++;
	}

	return count;
}

void SessionManager::on_connected(SessionRef session)
{
	add_session(session);
}

void SessionManager::on_disconnected(SessionRef session)
{
	release_session(session);
}
