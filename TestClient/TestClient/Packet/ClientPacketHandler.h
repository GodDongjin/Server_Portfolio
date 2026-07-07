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
	PKT_REQ_GET_ROOM_INFO = 1006,
	PKT_ACK_GET_ROOM_INFO = 1007,
	PKT_REQ_ENTER_ROOM = 1008,
	PKT_ACK_ENTER_ROOM = 1009,
	PKT_REQ_EXIT_ROOM = 1010,
	PKT_ACK_EXIT_ROOM = 1011,
	PKT_REQ_CHAT = 1012,
	PKT_ACK_CHAT = 1013,
	PKT_ACK_SEND_CHAT = 1014,
	PKT_REQ_CONNECT_PONG = 1015,
	PKT_ACK_SEND_CONNECT_PING = 1016,
};

bool Handle_ACK_LOGIN(shared_ptr<TestSession>& session, Protocol::ACK_LOGIN& pkt);
bool Handle_ACK_BOT_LOGIN(shared_ptr<TestSession>& session, Protocol::ACK_BOT_LOGIN& pkt);
bool Handle_ACK_LOGOUT(shared_ptr<TestSession>& session, Protocol::ACK_LOGOUT& pkt);
bool Handle_ACK_GET_ROOM_INFO(shared_ptr<TestSession>& session, Protocol::ACK_GET_ROOM_INFO& pkt);
bool Handle_ACK_ENTER_ROOM(shared_ptr<TestSession>& session, Protocol::ACK_ENTER_ROOM& pkt);
bool Handle_ACK_EXIT_ROOM(shared_ptr<TestSession>& session, Protocol::ACK_EXIT_ROOM& pkt);
bool Handle_ACK_CHAT(shared_ptr<TestSession>& session, Protocol::ACK_CHAT& pkt);
bool Handle_ACK_SEND_CHAT(shared_ptr<TestSession>& session, Protocol::ACK_SEND_CHAT& pkt);
bool Handle_ACK_SEND_CONNECT_PING(shared_ptr<TestSession>& session, Protocol::ACK_SEND_CONNECT_PING& pkt);

class ClientPacketHandler
{
public:
	static void Init()
	{
		for (int32 i = 0; i < UINT16_MAX; i++)
			GPacketHandler[i] = nullptr;
		GPacketHandler[PKT_ACK_LOGIN] = [](shared_ptr<TestSession>& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::ACK_LOGIN>(Handle_ACK_LOGIN, session, buffer, len); };
		GPacketHandler[PKT_ACK_BOT_LOGIN] = [](shared_ptr<TestSession>& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::ACK_BOT_LOGIN>(Handle_ACK_BOT_LOGIN, session, buffer, len); };
		GPacketHandler[PKT_ACK_LOGOUT] = [](shared_ptr<TestSession>& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::ACK_LOGOUT>(Handle_ACK_LOGOUT, session, buffer, len); };
		GPacketHandler[PKT_ACK_GET_ROOM_INFO] = [](shared_ptr<TestSession>& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::ACK_GET_ROOM_INFO>(Handle_ACK_GET_ROOM_INFO, session, buffer, len); };
		GPacketHandler[PKT_ACK_ENTER_ROOM] = [](shared_ptr<TestSession>& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::ACK_ENTER_ROOM>(Handle_ACK_ENTER_ROOM, session, buffer, len); };
		GPacketHandler[PKT_ACK_EXIT_ROOM] = [](shared_ptr<TestSession>& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::ACK_EXIT_ROOM>(Handle_ACK_EXIT_ROOM, session, buffer, len); };
		GPacketHandler[PKT_ACK_CHAT] = [](shared_ptr<TestSession>& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::ACK_CHAT>(Handle_ACK_CHAT, session, buffer, len); };
		GPacketHandler[PKT_ACK_SEND_CHAT] = [](shared_ptr<TestSession>& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::ACK_SEND_CHAT>(Handle_ACK_SEND_CHAT, session, buffer, len); };
		GPacketHandler[PKT_ACK_SEND_CONNECT_PING] = [](shared_ptr<TestSession>& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::ACK_SEND_CONNECT_PING>(Handle_ACK_SEND_CONNECT_PING, session, buffer, len); };
	}

	static bool HandlePacket(shared_ptr<TestSession>& session, BYTE* buffer, int32 len)
	{
		PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);

		if (header->id >= UINT16_MAX)
			return false;

		PacketHandlerFunc handler = GPacketHandler[header->id];

		if (handler == nullptr)
		{
			wcout << L"HandlePacket ERROR - ID : " << header->id << endl;
			return false;
		}

		return handler(session, buffer, len);
	}
	static shared_ptr<SendBuffer> MakeSendBuffer(Protocol::REQ_LOGIN& pkt) { return MakeSendBuffer(pkt, PKT_REQ_LOGIN); }
	static shared_ptr<SendBuffer> MakeSendBuffer(Protocol::REQ_BOT_LOGIN& pkt) { return MakeSendBuffer(pkt, PKT_REQ_BOT_LOGIN); }
	static shared_ptr<SendBuffer> MakeSendBuffer(Protocol::REQ_LOGOUT& pkt) { return MakeSendBuffer(pkt, PKT_REQ_LOGOUT); }
	static shared_ptr<SendBuffer> MakeSendBuffer(Protocol::REQ_GET_ROOM_INFO& pkt) { return MakeSendBuffer(pkt, PKT_REQ_GET_ROOM_INFO); }
	static shared_ptr<SendBuffer> MakeSendBuffer(Protocol::REQ_ENTER_ROOM& pkt) { return MakeSendBuffer(pkt, PKT_REQ_ENTER_ROOM); }
	static shared_ptr<SendBuffer> MakeSendBuffer(Protocol::REQ_EXIT_ROOM& pkt) { return MakeSendBuffer(pkt, PKT_REQ_EXIT_ROOM); }
	static shared_ptr<SendBuffer> MakeSendBuffer(Protocol::REQ_CHAT& pkt) { return MakeSendBuffer(pkt, PKT_REQ_CHAT); }
	static shared_ptr<SendBuffer> MakeSendBuffer(Protocol::REQ_CONNECT_PONG& pkt) { return MakeSendBuffer(pkt, PKT_REQ_CONNECT_PONG); }

private:
	template<typename PacketType, typename ProcessFunc>
	static bool HandlePacket(ProcessFunc func, shared_ptr<TestSession>& session, BYTE* buffer, int32 len)
	{
		PacketType pkt;
		if (pkt.ParseFromArray(buffer + sizeof(PacketHeader), len - sizeof(PacketHeader)) == false)
		{
			wcout << L"Parse packet failed" << endl;
			return false;
		}

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
