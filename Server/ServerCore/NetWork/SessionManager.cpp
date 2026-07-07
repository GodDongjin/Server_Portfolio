#include "pch.h"
#include "SessionManager.h"

SessionManager::SessionManager(int32 sessionMaxCount)
	: _max_session_count(sessionMaxCount), _session_count(0)
{
}

void SessionManager::add_session(SessionRef session)
{
	if (session == nullptr) {
		return;
	}
		

	bool need_process_connect = false;

	{
		WRITE_LOCK;

		auto [iter, inserted] = _sessions.emplace(session);

		if (inserted)
		{
			_session_count++;
			need_process_connect = true;
		}
	}

	if (need_process_connect) {
		session->set_process_connect();
	}
}

void SessionManager::add_account_session(uint64 account_id, SessionRef session)
{
	if (account_id == 0 || session == nullptr) {
		return;
	}

	WRITE_LOCK;
	_account_sessions[account_id] = session;
}

void SessionManager::add_name_session(wstring name, SessionRef session)
{
	if (name.empty() || session == nullptr) {
		return;
	}

	WRITE_LOCK;
	_name_sessions[name] = session;
}

void SessionManager::release_session(SessionRef session)
{
	if (session == nullptr)
		return;

	WRITE_LOCK;

	const uint64 account_id = session->get_account_idx();

	if (account_id != 0) {
		_account_sessions.erase(account_id);
	}

	if (!session->get_name().empty()) {
		_name_sessions.erase(session->get_name());
	}

	const size_t erased = _sessions.erase(session);

	if (erased > 0)
		_session_count--;

	session->set_account_idx(0);
}

void SessionManager::release_logout_session(uint64 account_id)
{
	if (account_id == 0)
		return;

	WRITE_LOCK;
	_account_sessions.erase(account_id);
}

void SessionManager::release_name_session(wstring user_name)
{
	if (user_name == L"")
		return;

	WRITE_LOCK;
	_name_sessions.erase(user_name);
}

bool SessionManager::is_whisper_target_find(wstring target_name)
{
	WRITE_LOCK;

	auto iter = _name_sessions.find(target_name);
	if (iter == _name_sessions.end()) {
		return false;
	}
		
	return  true;
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

// 클라이언트 통신 상태 확인
void SessionManager::check_heartbeat(uint64 now)
{
	//pong 응답 대기 시간 15초로 설정
	constexpr uint64 PONG_TIMEOUT_MS = 15000;
	vector<SessionRef> sessions;

	{
		WRITE_LOCK;
		sessions.assign(_sessions.begin(), _sessions.end());
	}

	for (SessionRef& session : sessions)
	{
		if (session == nullptr || !session->is_connected())
			continue;

		// pong 응답 대기중인지 채크
		if (session->get_waiting_pong())
		{
			// 대기중이면 현재 대기중인 시간 채크
			if (now - session->get_last_ping_tick() > PONG_TIMEOUT_MS)
			{
				// pong 응답 대기시간이 제한 시간을 넘으면 비정상이라 판단하여 disconnect 진행
				ERROR_LOG("heartbeat timeout");
				session->disconnect();
			}

			continue;
		}

		session->send_ping(now);
	}
}

int32 SessionManager::all_chat(SendBufferRef sendBuffer)
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
		if (session->is_connected() == false || session->get_room_id() < 0) {
			continue;
		}

		session->send(sendBuffer);
		count++;
	}

	return count;
}

int32 SessionManager::normal_chat(int32 room_id, SendBufferRef sendBuffer)
{
	vector<SessionRef> sessions;

	{
		WRITE_LOCK;

		for (auto& [account_id, session] : _account_sessions)
		{
			if (session && session->is_connected() && session->get_room_id() == room_id) {
				sessions.push_back(session);
			}		
		}
	}

	int32 count = 0;

	for (SessionRef& session : sessions)
	{
		session->send(sendBuffer);
		count++;
	}

	return count;
}

int32 SessionManager::whisper_chat(wstring target_name, SendBufferRef sendBuffer)
{
	int32 count = 0;

	SessionRef session;

	{
		WRITE_LOCK;

		auto iter = _name_sessions.find(target_name);
		if (iter == _name_sessions.end())
			return 0;

		session = iter->second;
	}

	if (session == nullptr || session->is_connected() == false)
		return 0;

	session->send(sendBuffer);
	count++;
	
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


