#include "pch.h"
#include "GameSession.h"
#include "ServerPacketHandler.h"

void GameSession::on_send(int32 len)
{
}

void GameSession::on_recv_packet(BYTE* buffer, int32 len)
{
	SessionRef session = get_session();
	PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);

	if (!ServerPacketHandler::HandlePacket(session, buffer, len))
	{
		//PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);

		//ERROR_LOG("HandlePacket ERROR - ID : %d", header->id);
	}
}
