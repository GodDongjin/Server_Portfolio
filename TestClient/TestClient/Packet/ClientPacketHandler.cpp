#include "ClientPacketHandler.h"

PacketHandlerFunc GPacketHandler[UINT16_MAX];


bool Handle_INVALID(shared_ptr<TestSession>& session, BYTE* buffer, int32 len)
{
    return true;
}

bool Handle_ACK_LOGIN(shared_ptr<TestSession>& session, Protocol::ACK_LOGIN& pkt)
{
    return true;
}

bool Handle_ACK_CHAR(shared_ptr<TestSession>& session, Protocol::ACK_CHAR& pkt)
{
    cout << "ACK message : " << pkt.message() << endl;
    return true;
}
