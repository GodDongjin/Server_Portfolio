#pragma once
#include "../Main/GameSession.h"
#include "../Protobuf/Protocol.pb.h"
#include "../ServerCore/NetWork/SendBuffer.h"
#include <utility>

using PacketHandlerFunc = std::function<bool(GameSessionRef&, BYTE*, int32)>;
extern PacketHandlerFunc GPacketHandler[UINT16_MAX];

enum : uint16
{
{%- for pkt in parser.total_pkt %}
	PKT_{{pkt.name}} = {{pkt.id}},
{%- endfor %}
};

{%- for pkt in parser.recv_pkt %}
bool Handle_{{pkt.name}}(GameSessionRef& session, Protocol::{{pkt.name}}& pkt);
{%- endfor %}

class {{output}}
{
public:
	static void Init()
	{
		for (int32 i = 0; i < UINT16_MAX; i++)
			GPacketHandler[i] = nullptr;

{%- for pkt in parser.recv_pkt %}
		GPacketHandler[PKT_{{pkt.name}}] = [](GameSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::{{pkt.name}}>(Handle_{{pkt.name}}, session, buffer, len); };
{%- endfor %}
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

{%- for pkt in parser.send_pkt %}
	static SendBufferRef MakeSendBuffer(Protocol::{{pkt.name}}& pkt) { return MakeSendBuffer(pkt, PKT_{{pkt.name}}); }
{%- endfor %}

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

