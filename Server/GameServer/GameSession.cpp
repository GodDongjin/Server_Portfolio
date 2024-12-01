#include "pch.h"
#include "GameSession.h"
#include "ServerPacketHandler.h"

void GameSession::OnSend(int32 len)
{
}

void GameSession::OnRecvPacket(BYTE* buffer, int32 len)
{
	SessionRef session = GetSessionRef();
	PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);

	ServerPacketHandler::HandlePacket(session, buffer, len);
}
