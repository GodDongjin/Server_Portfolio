#pragma once
#include "../Main/GameSession.h"
#include "../Protobuf/Protocol.pb.h"
#include "../ServerCore/NetWork/SendBuffer.h"
#include <utility>

using PacketHandlerFunc = std::function<bool(GameSessionRef&, BYTE*, int32)>;
extern PacketHandlerFunc GPacketHandler[UINT16_MAX];

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
	PKT_ACK_SERVER_FULL = 1017,
};
bool Handle_REQ_LOGIN(GameSessionRef& session, Protocol::REQ_LOGIN& pkt);
bool Handle_REQ_BOT_LOGIN(GameSessionRef& session, Protocol::REQ_BOT_LOGIN& pkt);
bool Handle_REQ_LOGOUT(GameSessionRef& session, Protocol::REQ_LOGOUT& pkt);
bool Handle_REQ_GET_ROOM_INFO(GameSessionRef& session, Protocol::REQ_GET_ROOM_INFO& pkt);
bool Handle_REQ_ENTER_ROOM(GameSessionRef& session, Protocol::REQ_ENTER_ROOM& pkt);
bool Handle_REQ_EXIT_ROOM(GameSessionRef& session, Protocol::REQ_EXIT_ROOM& pkt);
bool Handle_REQ_CHAT(GameSessionRef& session, Protocol::REQ_CHAT& pkt);
bool Handle_REQ_CONNECT_PONG(GameSessionRef& session, Protocol::REQ_CONNECT_PONG& pkt);

class ServerPacketHandler
{
public:
	static void Init()
	{
		for (int32 i = 0; i < UINT16_MAX; i++)
			GPacketHandler[i] = nullptr;
		GPacketHandler[PKT_REQ_LOGIN] = [](GameSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::REQ_LOGIN>(Handle_REQ_LOGIN, session, buffer, len); };
		GPacketHandler[PKT_REQ_BOT_LOGIN] = [](GameSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::REQ_BOT_LOGIN>(Handle_REQ_BOT_LOGIN, session, buffer, len); };
		GPacketHandler[PKT_REQ_LOGOUT] = [](GameSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::REQ_LOGOUT>(Handle_REQ_LOGOUT, session, buffer, len); };
		GPacketHandler[PKT_REQ_GET_ROOM_INFO] = [](GameSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::REQ_GET_ROOM_INFO>(Handle_REQ_GET_ROOM_INFO, session, buffer, len); };
		GPacketHandler[PKT_REQ_ENTER_ROOM] = [](GameSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::REQ_ENTER_ROOM>(Handle_REQ_ENTER_ROOM, session, buffer, len); };
		GPacketHandler[PKT_REQ_EXIT_ROOM] = [](GameSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::REQ_EXIT_ROOM>(Handle_REQ_EXIT_ROOM, session, buffer, len); };
		GPacketHandler[PKT_REQ_CHAT] = [](GameSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::REQ_CHAT>(Handle_REQ_CHAT, session, buffer, len); };
		GPacketHandler[PKT_REQ_CONNECT_PONG] = [](GameSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::REQ_CONNECT_PONG>(Handle_REQ_CONNECT_PONG, session, buffer, len); };
	}

	static bool HandlePacket(GameSessionRef& session, BYTE* buffer, int32 len)
	{
		if (!session || !buffer || len < static_cast<int32>(sizeof(PacketHeader)))
			return false;

		PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);

		if (header->id >= UINT16_MAX)
			return false;

		PacketHandlerFunc handler = GPacketHandler[header->id];

		if (handler == nullptr)
		{
			ERROR_LOG("Invalid packet id");
			return false;
		}

		return handler(session, buffer, len);
	}
	static SendBufferRef MakeSendBuffer(Protocol::ACK_LOGIN& pkt) { return MakeSendBuffer(pkt, PKT_ACK_LOGIN); }
	static SendBufferRef MakeSendBuffer(Protocol::ACK_BOT_LOGIN& pkt) { return MakeSendBuffer(pkt, PKT_ACK_BOT_LOGIN); }
	static SendBufferRef MakeSendBuffer(Protocol::ACK_LOGOUT& pkt) { return MakeSendBuffer(pkt, PKT_ACK_LOGOUT); }
	static SendBufferRef MakeSendBuffer(Protocol::ACK_GET_ROOM_INFO& pkt) { return MakeSendBuffer(pkt, PKT_ACK_GET_ROOM_INFO); }
	static SendBufferRef MakeSendBuffer(Protocol::ACK_ENTER_ROOM& pkt) { return MakeSendBuffer(pkt, PKT_ACK_ENTER_ROOM); }
	static SendBufferRef MakeSendBuffer(Protocol::ACK_EXIT_ROOM& pkt) { return MakeSendBuffer(pkt, PKT_ACK_EXIT_ROOM); }
	static SendBufferRef MakeSendBuffer(Protocol::ACK_CHAT& pkt) { return MakeSendBuffer(pkt, PKT_ACK_CHAT); }
	static SendBufferRef MakeSendBuffer(Protocol::ACK_SEND_CHAT& pkt) { return MakeSendBuffer(pkt, PKT_ACK_SEND_CHAT); }
	static SendBufferRef MakeSendBuffer(Protocol::ACK_SEND_CONNECT_PING& pkt) { return MakeSendBuffer(pkt, PKT_ACK_SEND_CONNECT_PING); }
	static SendBufferRef MakeSendBuffer(Protocol::ACK_SERVER_FULL& pkt) { return MakeSendBuffer(pkt, PKT_ACK_SERVER_FULL); }

private:
	template<typename PacketType, typename ProcessFunc>
	static bool HandlePacket(ProcessFunc func, GameSessionRef& session, BYTE* buffer, int32 len)
	{
		PacketType pkt;
		if (pkt.ParseFromArray(buffer + sizeof(PacketHeader), len - sizeof(PacketHeader)) == false)
		{
			ERROR_LOG("Parse packet failed");
			return false;
		}

		// 수신 버퍼는 이 함수가 끝난 뒤 재사용되므로 버퍼 주소를 작업에 넘기지 않는다.
		// 파싱한 패킷은 이동하고, 세션 shared_ptr는 복사해 큐 작업이 각각 보관한다.
		session->post_logic([self = session, func, pkt = std::move(pkt)]() mutable
		{
			// 큐에서 기다리는 동안 연결이 끊겼다면 이 패킷은 처리하지 않는다.
			if (!self->is_connected())
				return;

			if (!func(self, pkt))
				ERROR_LOG("Packet handler failed");
		});

		// true는 파싱과 큐 등록 성공을 의미한다. 실제 핸들러 처리는 이후에 실행된다.
		return true;
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
