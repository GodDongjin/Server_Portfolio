#include "pch.h"
#include "SessionManager.h"

SessionManager::SessionManager(int32 sessionMaxCount)
	: _max_session_count(sessionMaxCount), _session_count(0)
{
}

bool SessionManager::add_session(SessionRef session)
{
	if (session == nullptr) {
		return false;
	}

	{
		WRITE_LOCK;

		if (_sessions.count(session) > 0)
			return true;

		// 정원 확인과 등록을 같은 잠금 안에서 처리해 동시 접속 시 정원 초과를 막는다.
		if (_session_count >= _max_session_count) {
			return false;
		}

		_sessions.emplace(session);
		_session_count++;
	}

	// 등록에 성공한 세션만 수신을 시작한다.
	session->set_process_connect();
	return true;
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

	// 해당 세션의 작업 큐에서 호출하므로 계정 ID와 이름을 안전하게 읽을 수 있다.
	// 각 등록 해제 함수가 내부에서 잠금을 잡고 풀기 때문에 바깥에서 중복으로 잠그지 않는다.
	release_logout_session(session->get_account_idx(), session);
	release_name_session(session->get_name(), session);

	WRITE_LOCK;
	if (_sessions.erase(session) > 0) {
		_session_count--;
	}
}

void SessionManager::release_logout_session(uint64 account_id, const SessionRef& expected_session)
{
	if (account_id == 0 || !expected_session) {
		return;
	}

	WRITE_LOCK;

	// 이전 세션의 정리가 늦어져도 새로 로그인한 세션의 등록을 삭제하지 않도록 소유자를 확인한다.
	auto iter = _account_sessions.find(account_id);
	if (iter != _account_sessions.end() && iter->second == expected_session) 
	{
		_account_sessions.erase(iter);
	}
}

void SessionManager::release_name_session(const wstring& user_name, const SessionRef& expected_session)
{
	if (user_name.empty() || !expected_session) {
		return;
	}
		

	WRITE_LOCK;
	// 현재 등록된 세션이 정리를 요청한 세션과 같을 때만 이름 등록을 삭제한다.
	auto iter = _name_sessions.find(user_name);
	if (iter != _name_sessions.end() && iter->second == expected_session) 
	{
		_name_sessions.erase(iter);
	}
		
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
		if (session->is_connected() == false) {
			continue;
		}

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
		if (session == nullptr || !session->is_connected()) {
			continue;
		}

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
		if (iter == _name_sessions.end()) {
			return 0;
		}
			

		session = iter->second;
	}

	if (session == nullptr || session->is_connected() == false) {
		return 0;
	}

	session->send(sendBuffer);
	count++;
	
	return count;
}

bool SessionManager::on_connected(SessionRef session)
{
	return add_session(session);
}

void SessionManager::on_disconnected(SessionRef session)
{
	release_session(session);
}


