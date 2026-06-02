#pragma once

#define WIN32_LEAN_AND_MEAN

// Types.h
#ifdef _DEBUG
#pragma comment(lib, "../Libraries/Libs/Protobuf/Debug/libprotobufd.lib")
#else
#pragma comment(lib, "../Libraries/Libs/Protobuf/Release/libprotobuf.lib")
#endif


#include <winsock2.h>
#include <ws2tcpip.h>
#include <mswsock.h>
#include <windows.h>

#include <array>
#include <vector>
#include <list>
#include <queue>
#include <stack>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <assert.h>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <ctime>

#include <memory>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

using BYTE = unsigned char;
using int8 = __int8;
using int16 = __int16;
using int32 = __int32;
using int64 = __int64;
using uint8 = unsigned __int8;
using uint16 = unsigned __int16;
using uint32 = unsigned __int32;
using uint64 = unsigned __int64;

constexpr int32 MAX_PACKET_SIZE = 0x10000;

#define USING_SHARED_PTR(name)	using name##Ref = std::shared_ptr<class name>;

#define size16(val)		static_cast<int16>(sizeof(val))
#define size32(val)		static_cast<int32>(sizeof(val))
#define len16(arr)		static_cast<int16>(sizeof(arr)/sizeof(arr[0]))
#define len32(arr)		static_cast<int32>(sizeof(arr)/sizeof(arr[0]))
