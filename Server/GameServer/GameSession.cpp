#include "pch.h"
#include "GameSession.h"
#include "ServerPacketHandler.h"
#include "GameGlobal.h"
#include "Login.h"

void GameSession::on_disconnect()
{
	if (_is_login)
	{
		GLogin->logout(get_account_idx());
		set_account_idx(0);
		_is_login = false;
	}
}

void GameSession::on_send(int32 len)
{
}

void GameSession::on_recv_packet(BYTE* buffer, int32 len)
{
	GameSessionRef session = get_game_session();
	PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);

	if (!ServerPacketHandler::HandlePacket(session, buffer, len))
	{
		PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);

		cout << "HandlePacket ERROR - ID : " << header->id << endl;
	}
}
