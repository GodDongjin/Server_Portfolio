#include "pch.h"
#include "SessionManager.h"

SessionManager::SessionManager(int32 sessionMaxCount)
	: _max_session_count(sessionMaxCount)
{
}

void SessionManager::add_session(SessionRef session)
{
	WRITE_LOCK;
	_sessions.insert(session);
	session->set_account_idx(_session_id.fetch_add(1));
	session->set_process_connect();
	_sessionCount++;
}

void SessionManager::release_session(SessionRef session)
{
	WRITE_LOCK;
	ASSERT_CRASH(_sessions.erase(session) != 0);
	_sessionCount--;
}

bool SessionManager::broad_cast(SendBufferRef sendBuffer)
{
	WRITE_LOCK;

	if (_sessions.empty())
		return false;

	for (auto session : _sessions)
	{
		session->send(sendBuffer);
	}

	return true;
}

void SessionManager::on_connected(SessionRef session)
{
	cout << "Success connect" << endl;
	add_session(session);
}

void SessionManager::on_disconnected(SessionRef session)
{
	cout << "Disconnected" << endl;
	release_session(session);
}
