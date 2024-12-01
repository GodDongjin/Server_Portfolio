#pragma once
#include "Player.h"


class PlayerManager : public enable_shared_from_this<PlayerManager>
{
public:
	PlayerManager();
	~PlayerManager();

public:
	PlayerRef GetPlayer(uint64 idx) { return players->find(idx)->second; }
	Protocol::PlayerInfo GetPlayerInfo(uint64 idx) { return players->find(idx)->second->GetPlayerInfo(); }
	bool FindPlayer(uint64 idx) { return players->find(idx) != players->end(); }

	shared_ptr<map<uint64, PlayerRef>> GetPlayersMap() { return players; }
	bool GetPlayerEmpty() { return (!players || players->empty()); }

public:
	void CreatePlayer(uint64 idx, SessionRef session, Protocol::PlayerInfo playerInfo);
	void DeletePlayer(uint64 playerId);

	bool MovePlayer(Protocol::PlayerPos pos, SessionRef mySession);
	void UserEnterBoradCast(Protocol::PlayerInfo playerInfo, SessionRef mySession);
	void BoradCast(SendBufferRef sendBuffer, uint64 idx, SessionRef mySession);
private:
	void DeleteAllPlayer();
private:
	USE_LOCK;
	shared_ptr<map<uint64/*sessionIDX*/, PlayerRef/*Player*/>> players;
};

extern PlayerManagerRef GPlayerManager;