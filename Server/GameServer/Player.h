#pragma once
#include "Session.h"


class Player
{
	enum class HeroType : BYTE
	{
		NONE,
		WARRIOR,
		WIZARD,
		ASSASSSIN,
	};

public:
	Player(SessionRef owner, Protocol::PlayerInfo playerInfo);
	~Player();

public:
	Protocol::PlayerInfo GetPlayerInfo() { return mPlayerInfo; }
	void SetPlayerInfo(Protocol::PlayerInfo playerInfo) { mPlayerInfo = playerInfo; }
	bool GetInGameState() { return inGame; }
	SessionRef GetSeeion() { return mOwner; }
public:
	void PlayerEnterGame() { inGame = true; }
	void SetPlayerPos(Protocol::PlayerPos pos);
	Protocol::PlayerPos GetPos();
private:
	Protocol::PlayerInfo mPlayerInfo;
	HeroType mHeroType;
	SessionRef mOwner;
	bool inGame = false;
};

