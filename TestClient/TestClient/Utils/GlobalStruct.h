#pragma once
#include "Types.h"

struct ServerStats
{
    atomic<uint64> connect_success = 0;
    atomic<uint64> login_success = 0;
    atomic<uint64> send_chat = 0;
    atomic<uint64> recv_chat = 0;
    atomic<uint64> expected_chat_recv = 0;
    atomic<uint64> disconnect = 0;
};

extern ServerStats GServerStats;