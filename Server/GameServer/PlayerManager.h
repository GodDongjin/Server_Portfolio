#pragma once
#include "Player.h"


class PlayerManager : public enable_shared_from_this<PlayerManager>
{
public:
	PlayerManager();
	~PlayerManager();

public:
//	PlayerRef GetPlayer(uint64 idx) { return _player_list->find(idx)->second; }
//	Protocol::PlayerInfo GetPlayerInfo(uint64 idx) { return _player_list->find(idx)->second->GetPlayerInfo(); }
//	bool FindPlayer(uint64 idx) { return _player_list->find(idx) != _player_list->end(); }
//
//	shared_ptr<map<uint64, PlayerRef>> GetPlayersMap() { return _player_list; }
//	bool GetPlayerEmpty() { return (!_player_list || _player_list->empty()); }
//
//public:
//	void CreatePlayer(uint64 idx, SessionRef session, Protocol::PlayerInfo playerInfo);
//	void DeletePlayer(uint64 playerId);
//
//	bool MovePlayer(Protocol::PlayerPos pos, SessionRef mySession);
//	void UserEnterBoradCast(Protocol::PlayerInfo playerInfo, SessionRef mySession);
//	void BoradCast(SendBufferRef sendBuffer, uint64 idx, SessionRef mySession);
//private:
//	void DeleteAllPlayer();
//private:
//	USE_LOCK;
//	shared_ptr<map<uint64/*sessionIDX*/, PlayerRef/*Player*/>> _player_list;
};

extern PlayerManagerRef GPlayerManager;