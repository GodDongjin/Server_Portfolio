#pragma once
#include "Protocol.pb.h"
#include "Session.h"
#include "SendBuffer.h"

using PacketHandlerFunc = std::function<bool(SessionRef&, BYTE*, int32)>;
extern PacketHandlerFunc GPacketHandler[UINT16_MAX];

enum : uint16
{
	PKT_C_LOGIN = 1000,
	PKT_S_LOGIN = 1001,
	PKT_C_ACCESSION = 1002,
	PKT_S_ACCESSION = 1003,
	PKT_C_SELECT_ROOM_ENTER = 1004,
	PKT_S_SELECT_ROOM_ENTER = 1005,
	PKT_C_CREATE_PLAYER = 1006,
	PKT_S_CREATE_PLAYER = 1007,
	PKT_C_ENTER_GAME = 1008,
	PKT_S_ENTER_GAME = 1009,
	PKT_C_MOVE = 1010,
	PKT_S_MOVE = 1011,
	PKT_S_SEND_OTHER_USER = 1012,
	PKT_C_LEAVE_GAME = 1013,
	PKT_S_LEAVE_GAME = 1014,
	PKT_S_SPAWN = 1015,
	PKT_S_DESPAWN = 1016,
	PKT_C_CHAT = 1017,
	PKT_S_CHAT = 1018,
	PKT_C_HELLOWORLD = 1019,
};

bool Handle_INVALID(SessionRef& session, BYTE* buffer, int32 len);
bool Handle_C_LOGIN(SessionRef& session, Protocol::C_LOGIN& pkt);
bool Handle_C_ACCESSION(SessionRef& session, Protocol::C_ACCESSION& pkt);
bool Handle_C_SELECT_ROOM_ENTER(SessionRef& session, Protocol::C_SELECT_ROOM_ENTER& pkt);
bool Handle_C_CREATE_PLAYER(SessionRef& session, Protocol::C_CREATE_PLAYER& pkt);
bool Handle_C_ENTER_GAME(SessionRef& session, Protocol::C_ENTER_GAME& pkt);
bool Handle_C_MOVE(SessionRef& session, Protocol::C_MOVE& pkt);
bool Handle_C_LEAVE_GAME(SessionRef& session, Protocol::C_LEAVE_GAME& pkt);
bool Handle_C_CHAT(SessionRef& session, Protocol::C_CHAT& pkt);
bool Handle_C_HELLOWORLD(SessionRef& session, Protocol::C_HELLOWORLD& pkt);

class ServerPacketHandler
{
public:
	static void Init()
	{
		for (int32 i = 0; i < UINT16_MAX; i++)
			GPacketHandler[i] = Handle_INVALID;
		GPacketHandler[PKT_C_LOGIN] = [](SessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::C_LOGIN>(Handle_C_LOGIN, session, buffer, len); };
		GPacketHandler[PKT_C_ACCESSION] = [](SessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::C_ACCESSION>(Handle_C_ACCESSION, session, buffer, len); };
		GPacketHandler[PKT_C_SELECT_ROOM_ENTER] = [](SessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::C_SELECT_ROOM_ENTER>(Handle_C_SELECT_ROOM_ENTER, session, buffer, len); };
		GPacketHandler[PKT_C_CREATE_PLAYER] = [](SessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::C_CREATE_PLAYER>(Handle_C_CREATE_PLAYER, session, buffer, len); };
		GPacketHandler[PKT_C_ENTER_GAME] = [](SessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::C_ENTER_GAME>(Handle_C_ENTER_GAME, session, buffer, len); };
		GPacketHandler[PKT_C_MOVE] = [](SessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::C_MOVE>(Handle_C_MOVE, session, buffer, len); };
		GPacketHandler[PKT_C_LEAVE_GAME] = [](SessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::C_LEAVE_GAME>(Handle_C_LEAVE_GAME, session, buffer, len); };
		GPacketHandler[PKT_C_CHAT] = [](SessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::C_CHAT>(Handle_C_CHAT, session, buffer, len); };
		GPacketHandler[PKT_C_HELLOWORLD] = [](SessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::C_HELLOWORLD>(Handle_C_HELLOWORLD, session, buffer, len); };
	}

	static bool HandlePacket(SessionRef& session, BYTE* buffer, int32 len)
	{
		PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);
		return GPacketHandler[header->id](session, buffer, len);
	}
	static SendBufferRef MakeSendBuffer(Protocol::S_LOGIN& pkt) { return MakeSendBuffer(pkt, PKT_S_LOGIN); }
	static SendBufferRef MakeSendBuffer(Protocol::S_ACCESSION& pkt) { return MakeSendBuffer(pkt, PKT_S_ACCESSION); }
	static SendBufferRef MakeSendBuffer(Protocol::S_SELECT_ROOM_ENTER& pkt) { return MakeSendBuffer(pkt, PKT_S_SELECT_ROOM_ENTER); }
	static SendBufferRef MakeSendBuffer(Protocol::S_CREATE_PLAYER& pkt) { return MakeSendBuffer(pkt, PKT_S_CREATE_PLAYER); }
	static SendBufferRef MakeSendBuffer(Protocol::S_ENTER_GAME& pkt) { return MakeSendBuffer(pkt, PKT_S_ENTER_GAME); }
	static SendBufferRef MakeSendBuffer(Protocol::S_MOVE& pkt) { return MakeSendBuffer(pkt, PKT_S_MOVE); }
	static SendBufferRef MakeSendBuffer(Protocol::S_SEND_OTHER_USER& pkt) { return MakeSendBuffer(pkt, PKT_S_SEND_OTHER_USER); }
	static SendBufferRef MakeSendBuffer(Protocol::S_LEAVE_GAME& pkt) { return MakeSendBuffer(pkt, PKT_S_LEAVE_GAME); }
	static SendBufferRef MakeSendBuffer(Protocol::S_SPAWN& pkt) { return MakeSendBuffer(pkt, PKT_S_SPAWN); }
	static SendBufferRef MakeSendBuffer(Protocol::S_DESPAWN& pkt) { return MakeSendBuffer(pkt, PKT_S_DESPAWN); }
	static SendBufferRef MakeSendBuffer(Protocol::S_CHAT& pkt) { return MakeSendBuffer(pkt, PKT_S_CHAT); }

private:
	template<typename PacketType, typename ProcessFunc>
	static bool HandlePacket(ProcessFunc func, SessionRef& session, BYTE* buffer, int32 len)
	{
		PacketType pkt;
		if (pkt.ParseFromArray(buffer + sizeof(PacketHeader), len - sizeof(PacketHeader)) == false)
			return false;

		return func(session, pkt);
	}

	template<typename T>
	static SendBufferRef MakeSendBuffer(T& pkt, uint16 pktId)
	{
		const uint16 dataSize = static_cast<uint16>(pkt.ByteSizeLong());
		const uint16 packetSize = dataSize + sizeof(PacketHeader);

		SendBufferRef sendBuffer = make_shared<SendBuffer>(packetSize);
		PacketHeader* header = reinterpret_cast<PacketHeader*>(sendBuffer->Buffer());
		header->size = packetSize;
		header->id = pktId;
		pkt.SerializeToArray(&header[1], dataSize);
		sendBuffer->Close(packetSize);

		return sendBuffer;
	}
};
