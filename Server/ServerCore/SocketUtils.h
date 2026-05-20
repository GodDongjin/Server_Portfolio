#pragma once
#include "NetAddress.h"

/*----------------
	SocketUtils
-----------------*/

class SocketUtils
{
public:
	static LPFN_CONNECTEX		ConnectEx;
	static LPFN_DISCONNECTEX	DisconnectEx;
	static LPFN_ACCEPTEX		AcceptEx;

public:
	static void init();
	static void clear();

	static bool bind_windows_function(SOCKET socket, GUID guid, LPVOID* fn);
	static SOCKET create_socket();

	static bool set_linger(SOCKET socket, uint16 onoff, uint16 linger);
	static bool set_resuse_address(SOCKET socket, bool flag);
	static bool set_recv_buffer_size(SOCKET socket, int32 size);
	static bool set_send_buffer_size(SOCKET socket, int32 size);
	static bool set_tcp_no_delay(SOCKET socket, bool flag);
	static bool set_update_accept_socket(SOCKET socket, SOCKET listenSocket);

	static bool bind(SOCKET socket, NetAddress netAddr);
	static bool bind_any_address(SOCKET socket, uint16 port);
	static bool listen(SOCKET socket, int32 backlog = SOMAXCONN);
	static void close(SOCKET& socket);
};

template<typename T>
static inline bool set_socket_opt(SOCKET socket, int32 level, int32 optName, T optVal)
{
	return SOCKET_ERROR != ::setsockopt(socket, level, optName, reinterpret_cast<char*>(&optVal), sizeof(T));
}