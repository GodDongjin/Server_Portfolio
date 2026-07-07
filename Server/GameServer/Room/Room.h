#pragma once
#include "../GameServer/Main/GameSession.h"

class Room
{
public:
	Room(BYTE room_id, int32 room_max_count) { _room_id = room_id; _room_max_count = room_max_count; };
	~Room() {};

	bool enter_user(uint64 account_id, shared_ptr<Session> session);
	bool exit_user(uint64 account_id);
	
public:
	BYTE get_room_id() { return _room_id; }
	int32 get_room_cur_count() { return _room_cur_count; }
	int32 get_room_max_count() { return _room_max_count; }

private:
	int32 _room_max_count;
	int32 _room_cur_count = 0;
	BYTE _room_id;
	map<uint64, shared_ptr<Session>> _room_session_list;

	USE_LOCK;
};