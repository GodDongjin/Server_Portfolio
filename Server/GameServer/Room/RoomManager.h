#pragma once
#include "Room.h"

#include "../ServerCore/Thread/ThreadManager.h"
#include "../ServerCore/Utils/Singleton.h"

class RoomManager : public Singleton<RoomManager>
{
public:
	RoomManager()
	{
		create_room();
	}

	~RoomManager()
	{
		_room_list.clear();
	}

	void create_room();

	bool enter_room(int32 enter_room_id, shared_ptr<Session> session);
	bool exit_room(int32 exit_room_id, uint64 account_id);

public:
	const map<int32, shared_ptr<Room>>& get_room_list() const{ return _room_list; }

private:
	void do_worker_job();

private:
	map<int32, shared_ptr<Room>> _room_list;
	int32 _room_size = 100;
	int32 _max_room_count = 10;
};
