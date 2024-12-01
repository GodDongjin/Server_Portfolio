#include "pch.h"
#include "SessionManager.h"

SessionManager::SessionManager(int32 sessionMaxCount)
	: mMaxSessionCount(sessionMaxCount)
{
}

void SessionManager::AddSession(SessionRef session)
{
	WRITE_LOCK;
	mSessions.insert(session);
	mMaxSessionCount++;
	session->SetProcessConnect();
}

void SessionManager::ReleaseSession(SessionRef session)
{
	WRITE_LOCK;
	ASSERT_CRASH(mSessions.erase(session) != 0);
	mMaxSessionCount--;
	//session->Disconnect();
}

bool SessionManager::Broadcast(SendBufferRef sendBuffer)
{
	WRITE_LOCK;

	if (mSessions.empty())
		return false;

	for (auto session : mSessions)
	{
		session->Send(sendBuffer);
	}

	return true;
}

void SessionManager::OnConnected(SessionRef session)
{
	cout << "Success Connect" << endl;
	AddSession(session);
}

void SessionManager::OnDisconnected(SessionRef session)
{
	cout << "Disconnected" << endl;
	ReleaseSession(session);
}
