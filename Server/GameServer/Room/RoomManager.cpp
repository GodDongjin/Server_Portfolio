#include "pch.h"
#include "RoomManager.h"

void RoomManager::create_room()
{
	for (int32 room_id = 0; room_id < _max_room_count; room_id++)
	{
		shared_ptr<Room> room = make_shared<Room>(room_id, _room_size);
		_room_list.emplace(room_id, room);
	}
}

bool RoomManager::enter_room(int32 enter_room_id, shared_ptr<Session> session)
{
	if (session == nullptr)
		return false;

	auto room_iter = _room_list.find(enter_room_id);
	if (room_iter == _room_list.end()) {
		return false;
	}

	shared_ptr<Room> room = room_iter->second;
	if (room == nullptr) {
		return false;
	}

	return room->enter_user(session->get_account_idx(), session);
}

bool RoomManager::exit_room(int32 exit_room_id, uint64 account_id)
{
	auto room_iter = _room_list.find(exit_room_id);
	if (room_iter == _room_list.end())
		return false;

	shared_ptr<Room> room = room_iter->second;
	if (room == nullptr)
		return false;

	return room->exit_user(account_id);
}

void RoomManager::do_worker_job()
{
}