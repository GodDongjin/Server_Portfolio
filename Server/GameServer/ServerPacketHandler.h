#pragma once
#include "Protocol.pb.h"
#include "../ServerCore/NetWork/Session.h"
#include "../ServerCore/NetWork/SendBuffer.h"

using PacketHandlerFunc = std::function<bool(SessionRef&, BYTE*, int32)>;
extern PacketHandlerFunc GPacketHandler[UINT16_MAX];

enum : uint16
{
	PKT_REQ_LOGIN = 1000,
	PKT_ACK_LOGIN = 1001,
	PKT_REQ_CHAT = 1002,
	PKT_ACK_CHAT = 1003,
};

bool Handle_INVALID(SessionRef& session, BYTE* buffer, int32 len);
bool Handle_REQ_LOGIN(SessionRef& session, Protocol::REQ_LOGIN& pkt);
bool Handle_REQ_CHAT(SessionRef& session, Protocol::REQ_CHAT& pkt);

class ServerPacketHandler
{
public:
	static void Init()
	{
		for (int32 i = 0; i < UINT16_MAX; i++)
			GPacketHandler[i] = Handle_INVALID;
		GPacketHandler[PKT_REQ_LOGIN] = [](SessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::REQ_LOGIN>(Handle_REQ_LOGIN, session, buffer, len); };
		GPacketHandler[PKT_REQ_CHAT] = [](SessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::REQ_CHAT>(Handle_REQ_CHAT, session, buffer, len); };
	}

	static bool HandlePacket(SessionRef& session, BYTE* buffer, int32 len)
	{
		PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);
		return GPacketHandler[header->id](session, buffer, len);
	}
	static SendBufferRef MakeSendBuffer(Protocol::ACK_LOGIN& pkt) { return MakeSendBuffer(pkt, PKT_ACK_LOGIN); }
	static SendBufferRef MakeSendBuffer(Protocol::ACK_CHAT& pkt) { return MakeSendBuffer(pkt, PKT_ACK_CHAT); }

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
		PacketHeader* header = reinterpret_cast<PacketHeader*>(sendBuffer->get_buffer());
		header->size = packetSize;
		header->id = pktId;
		pkt.SerializeToArray(&header[1], dataSize);
		sendBuffer->close(packetSize);

		return sendBuffer;
	}
};
