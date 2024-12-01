#include "pch.h"
#include "Player.h"

Player::Player(SessionRef owner, Protocol::PlayerInfo playerInfo)
	: mOwner(owner), mPlayerInfo(playerInfo)
{
	inGame = false;
}

Player::~Player()
{
}

Protocol::PlayerPos Player::GetPos()
{
	Protocol::PlayerPos pos;
	pos.set_x(mPlayerInfo.x());
	pos.set_y(mPlayerInfo.y());
	pos.set_z(mPlayerInfo.z());
	return pos;
}

void Player::SetPlayerPos(Protocol::PlayerPos pos)
{
	mPlayerInfo.set_x(pos.x());
	mPlayerInfo.set_y(pos.y());
	mPlayerInfo.set_z(pos.z());
}
