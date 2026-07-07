#include "pch.h"
#include "Room.h"

bool Room::enter_user(uint64 account_id, shared_ptr<Session> session)
{
	if (account_id < 0 || session == nullptr){
		return false;
	}

	WRITE_LOCK;

	if (_room_max_count <= _room_cur_count) {
		return false;
	}

	if (_room_session_list.find(account_id) != _room_session_list.end())
	{
		// 이미 방에 입장해있음.

		return false;
	}

	_room_session_list.emplace(account_id, session);
	_room_cur_count++;

	return true;
}

bool Room::exit_user(uint64 account_id)
{
	WRITE_LOCK;

	auto iter = _room_session_list.find(account_id);
	if (iter == _room_session_list.end())
		return false;

	_room_session_list.erase(iter);
	_room_cur_count--;

	return true;
}

