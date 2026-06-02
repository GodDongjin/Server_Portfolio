#pragma once
#include "../Protocol/Protocol.pb.h"
#include "../Buffer/SendBuffer.h"
#include "../Session/TestSession.h"

using PacketHandlerFunc = std::function<bool(shared_ptr<TestSession>&, BYTE*, int32)>;
extern PacketHandlerFunc GPacketHandler[UINT16_MAX];

struct PacketHeader
{
	uint16 size;
	uint16 id;
};

enum : uint16
{
	PKT_REQ_LOGIN = 1000,
	PKT_ACK_LOGIN = 1001,
	PKT_REQ_BOT_LOGIN = 1002,
	PKT_ACK_BOT_LOGIN = 1003,
	PKT_REQ_LOGOUT = 1004,
	PKT_ACK_LOGOUT = 1005,
	PKT_REQ_CHAT = 1006,
	PKT_ACK_CHAT = 1007,
	PKT_ACK_SEND_CHAT = 1008,
};

bool Handle_INVALID(shared_ptr<TestSession>& session, BYTE* buffer, int32 len);
bool Handle_ACK_LOGIN(shared_ptr<TestSession>& session, Protocol::ACK_LOGIN& pkt);
bool Handle_ACK_BOT_LOGIN(shared_ptr<TestSession>& session, Protocol::ACK_BOT_LOGIN& pkt);
bool Handle_ACK_LOGOUT(shared_ptr<TestSession>& session, Protocol::ACK_LOGOUT& pkt);
bool Handle_ACK_CHAT(shared_ptr<TestSession>& session, Protocol::ACK_CHAT& pkt);
bool Handle_ACK_SEND_CHAT(shared_ptr<TestSession>& session, Protocol::ACK_SEND_CHAT& pkt);

class ClientPacketHandler
{
public:
	static void Init()
	{
		for (int32 i = 0; i < UINT16_MAX; i++)
			GPacketHandler[i] = Handle_INVALID;
		GPacketHandler[PKT_ACK_LOGIN] = [](shared_ptr<TestSession>& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::ACK_LOGIN>(Handle_ACK_LOGIN, session, buffer, len); };
		GPacketHandler[PKT_ACK_BOT_LOGIN] = [](shared_ptr<TestSession>& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::ACK_BOT_LOGIN>(Handle_ACK_BOT_LOGIN, session, buffer, len); };
		GPacketHandler[PKT_ACK_LOGOUT] = [](shared_ptr<TestSession>& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::ACK_LOGOUT>(Handle_ACK_LOGOUT, session, buffer, len); };
		GPacketHandler[PKT_ACK_CHAT] = [](shared_ptr<TestSession>& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::ACK_CHAT>(Handle_ACK_CHAT, session, buffer, len); };
		GPacketHandler[PKT_ACK_SEND_CHAT] = [](shared_ptr<TestSession>& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::ACK_SEND_CHAT>(Handle_ACK_SEND_CHAT, session, buffer, len); };
	}

	static bool HandlePacket(shared_ptr<TestSession>& session, BYTE* buffer, int32 len)
	{
		PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);
		return GPacketHandler[header->id](session, buffer, len);
	}
	static shared_ptr<SendBuffer> MakeSendBuffer(Protocol::REQ_LOGIN& pkt) { return MakeSendBuffer(pkt, PKT_REQ_LOGIN); }
	static shared_ptr<SendBuffer> MakeSendBuffer(Protocol::REQ_BOT_LOGIN& pkt) { return MakeSendBuffer(pkt, PKT_REQ_BOT_LOGIN); }
	static shared_ptr<SendBuffer> MakeSendBuffer(Protocol::REQ_LOGOUT& pkt) { return MakeSendBuffer(pkt, PKT_REQ_LOGOUT); }
	static shared_ptr<SendBuffer> MakeSendBuffer(Protocol::REQ_CHAT& pkt) { return MakeSendBuffer(pkt, PKT_REQ_CHAT); }

private:
	template<typename PacketType, typename ProcessFunc>
	static bool HandlePacket(ProcessFunc func, shared_ptr<TestSession>& session, BYTE* buffer, int32 len)
	{
		PacketType pkt;
		if (pkt.ParseFromArray(buffer + sizeof(PacketHeader), len - sizeof(PacketHeader)) == false)
			return false;

		return func(session, pkt);
	}

	template<typename T>
	static shared_ptr<SendBuffer> MakeSendBuffer(T& pkt, uint16 pktId)
	{
		const uint16 dataSize = static_cast<uint16>(pkt.ByteSizeLong());
		const uint16 packetSize = dataSize + sizeof(PacketHeader);

		shared_ptr<SendBuffer> sendBuffer = make_shared<SendBuffer>(packetSize);
		PacketHeader* header = reinterpret_cast<PacketHeader*>(sendBuffer->get_buffer());
		header->size = packetSize;
		header->id = pktId;
		pkt.SerializeToArray(&header[1], dataSize);
		sendBuffer->close(packetSize);

		return sendBuffer;
	}
};
