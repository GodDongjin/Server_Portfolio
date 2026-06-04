#pragma once
#include "Types.h"

enum class ClientMode
{
    MANUAL,
    LOAD_TEST
};

struct LoadTestConfig
{
    int32 session_count = 1000;
    int32 bot_thread_count = 4;
    int32 worker_thread_count = 4;
    int32 chat_interval_ms = 2500;
    wstring ip = L"127.0.0.1";
    uint16 port = 7777;
};

extern ClientMode GClientMode;