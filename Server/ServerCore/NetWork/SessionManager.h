#pragma once
#include "Session.h"

class SessionManager
{
public:
	SessionManager(int32 sessionMaxCount);

public:
	void add_session(SessionRef session);
	void add_account_session(uint64 account_id, SessionRef session);
	void add_name_session(wstring name, SessionRef session);
	void release_session(SessionRef session);
	void release_logout_session(uint64 account_id);
	void release_name_session(wstring user_name);

	bool is_whisper_target_find(wstring target_name);

public:
	int32 broad_cast(SendBufferRef sendBuffer);
	void check_heartbeat(uint64 now);			// 클라이언트 통신 상태 확인

public:
	int32 all_chat(SendBufferRef sendBuffer);
	int32 normal_chat(int32 room_id, SendBufferRef sendBuffer);
	int32 whisper_chat(wstring target_name, SendBufferRef sendBuffer);

public:
	void on_connected(SessionRef session);
	void on_disconnected(SessionRef session);

public:
	int32 get_max_session_count() { return _max_session_count; }

private:
	USE_LOCK;

	set<SessionRef> _sessions;		//session list
	map<uint64, SessionRef> _account_sessions;	//로그인된 session list
	map<wstring, SessionRef> _name_sessions;	//귓속말을 위한 key: name, value: session list

	int32 _session_count;
	int32 _max_session_count;
};

