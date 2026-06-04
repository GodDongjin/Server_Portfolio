#pragma once

#include "Types.h"

struct ServerStats
{
    std::atomic<uint64> connect = 0;
    std::atomic<uint64> login = 0;
    std::atomic<uint64> recv_chat = 0;

    std::atomic<uint64> broadcast_packet = 0;
    std::atomic<uint64> broadcast_target = 0;

    std::atomic<uint64> send_complete = 0;
    std::atomic<uint64> send_complete_bytes = 0;

    std::atomic<uint64> disconnect = 0;
};

extern ServerStats GServerStats;