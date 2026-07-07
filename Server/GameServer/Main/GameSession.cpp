#include "pch.h"
#include "../Main/GameSession.h"
#include "../Packet/ServerPacketHandler.h"
#include "../Utils/GameGlobal.h"
#include "../Login/Login.h"
#include "../Room/RoomManager.h"

void GameSession::on_disconnect()
{
	GServerStats.disconnect++;

	if (_enter_room_id >= 0)
	{
		RoomManager::get_instance()->exit_room(_enter_room_id, get_account_idx());
		_enter_room_id = -1;
	}

	if (_is_login.exchange(false))
	{
		GLogin->logout(get_account_idx());
	}
}

void GameSession::on_send(int32 len)
{
	GServerStats.send_complete++;
	GServerStats.send_complete_bytes += len;
}

void GameSession::on_recv_packet(BYTE* buffer, int32 len)
{
	GameSessionRef session = get_game_session();
	PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);

	if (!ServerPacketHandler::HandlePacket(session, buffer, len))
	{
		PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);

		wcout << L"HandlePacket ERROR - ID : " << header->id << endl;
	}
}

void GameSession::send_ping(uint64 now)
{
	set_last_ping_tick(now);
	set_waiting_pong(true);

	Protocol::ACK_SEND_CONNECT_PING pkt;
	pkt.set_server_tick(now);

	SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(pkt);
	send(sendBuffer);
}

