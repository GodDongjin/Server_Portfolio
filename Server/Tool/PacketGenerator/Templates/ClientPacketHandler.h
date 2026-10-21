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
{%- for pkt in parser.total_pkt %}
	PKT_{{pkt.name}} = {{pkt.id}},
{%- endfor %}
};

{%- for pkt in parser.recv_pkt %}
bool Handle_{{pkt.name}}(shared_ptr<TestSession>& session, Protocol::{{pkt.name}}& pkt);
{%- endfor %}

class {{output}}
{
public:
	static void Init()
	{
		for (int32 i = 0; i < UINT16_MAX; i++)
			GPacketHandler[i] = nullptr;
{%- for pkt in parser.recv_pkt %}
		GPacketHandler[PKT_{{pkt.name}}] = [](shared_ptr<TestSession>& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::{{pkt.name}}>(Handle_{{pkt.name}}, session, buffer, len); };
{%- endfor %}
	}

	static bool HandlePacket(shared_ptr<TestSession>& session, BYTE* buffer, int32 len)
	{
		if (session == nullptr || buffer == nullptr || len < sizeof(PacketHeader)) {
			return false;
		}

		PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);

		if (header->size != len || header->id >= UINT16_MAX)
			return false;

		PacketHandlerFunc handler = GPacketHandler[header->id];

		if (handler == nullptr)
		{
			wcout << L"HandlePacket ERROR - ID : " << header->id << endl;
			return false;
		}

		return handler(session, buffer, len);
	}
{%- for pkt in parser.send_pkt %}
	static shared_ptr<SendBuffer> MakeSendBuffer(Protocol::{{pkt.name}}& pkt) { return MakeSendBuffer(pkt, PKT_{{pkt.name}}); }
{%- endfor %}

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
		const size_t dataSize = pkt.ByteSizeLong();
		// uint16로 줄이기 전에 헤더를 포함한 전체 길이를 검사한다.
		if (dataSize > UINT16_MAX - sizeof(PacketHeader)) {
			return nullptr;
		}
		const uint16 packetSize = static_cast<uint16>(dataSize + sizeof(PacketHeader));

		shared_ptr<SendBuffer> sendBuffer = make_shared<SendBuffer>(packetSize);
		PacketHeader* header = reinterpret_cast<PacketHeader*>(sendBuffer->get_buffer());
		header->size = packetSize;
		header->id = pktId;
		if (!pkt.SerializeToArray(&header[1], static_cast<int32>(dataSize))) {
			return nullptr;
		}
		sendBuffer->close(packetSize);

		return sendBuffer;
	}
};
