#include "pch.h"
#include "SocketUtils.h"

/*----------------
	SocketUtils
-----------------*/

LPFN_CONNECTEX		SocketUtils::ConnectEx = nullptr;
LPFN_DISCONNECTEX	SocketUtils::DisconnectEx = nullptr;
LPFN_ACCEPTEX		SocketUtils::AcceptEx = nullptr;

void SocketUtils::init()
{
	WSADATA wsaData;
	ASSERT_CRASH(::WSAStartup(MAKEWORD(2, 2), OUT & wsaData) == 0);

	/* 런타임에 주소 얻어오는 API */
	SOCKET dummySocket = create_socket();
	ASSERT_CRASH(bind_windows_function(dummySocket, WSAID_CONNECTEX, reinterpret_cast<LPVOID*>(&ConnectEx)));
	ASSERT_CRASH(bind_windows_function(dummySocket, WSAID_DISCONNECTEX, reinterpret_cast<LPVOID*>(&DisconnectEx)));
	ASSERT_CRASH(bind_windows_function(dummySocket, WSAID_ACCEPTEX, reinterpret_cast<LPVOID*>(&AcceptEx)));
	close(dummySocket);
}

void SocketUtils::clear()
{
	::WSACleanup();
}

bool SocketUtils::bind_windows_function(SOCKET socket, GUID guid, LPVOID* fn)
{
	DWORD bytes = 0;
	return SOCKET_ERROR != ::WSAIoctl(socket, SIO_GET_EXTENSION_FUNCTION_POINTER, &guid, sizeof(guid), fn, sizeof(*fn), OUT & bytes, NULL, NULL);
}

SOCKET SocketUtils::create_socket()
{
	return ::WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
}

bool SocketUtils::set_linger(SOCKET socket, uint16 onoff, uint16 linger)
{
	LINGER option;
	option.l_onoff = onoff;
	option.l_linger = linger;
	return set_socket_opt(socket, SOL_SOCKET, SO_LINGER, option);
}

bool SocketUtils::set_resuse_address(SOCKET socket, bool flag)
{
	return set_socket_opt(socket, SOL_SOCKET, SO_REUSEADDR, flag);
}

bool SocketUtils::set_recv_buffer_size(SOCKET socket, int32 size)
{
	return set_socket_opt(socket, SOL_SOCKET, SO_RCVBUF, size);
}

bool SocketUtils::set_send_buffer_size(SOCKET socket, int32 size)
{
	return set_socket_opt(socket, SOL_SOCKET, SO_SNDBUF, size);
}

bool SocketUtils::set_tcp_no_delay(SOCKET socket, bool flag)
{
	return set_socket_opt(socket, SOL_SOCKET, TCP_NODELAY, flag);
}

// ListenSocket의 특성을 ClientSocket에 그대로 적용
bool SocketUtils::set_update_accept_socket(SOCKET socket, SOCKET listenSocket)
{
	return set_socket_opt(socket, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, listenSocket);
}

bool SocketUtils::bind(SOCKET socket, NetAddress netAddr)
{
	return SOCKET_ERROR != ::bind(socket, reinterpret_cast<const SOCKADDR*>(&netAddr.get_sock_addr()), sizeof(SOCKADDR_IN));
}

bool SocketUtils::bind_any_address(SOCKET socket, uint16 port)
{
	SOCKADDR_IN myAddress;
	myAddress.sin_family = AF_INET;
	myAddress.sin_addr.s_addr = ::htonl(INADDR_ANY);
	myAddress.sin_port = ::htons(port);

	return SOCKET_ERROR != ::bind(socket, reinterpret_cast<const SOCKADDR*>(&myAddress), sizeof(myAddress));
}

bool SocketUtils::listen(SOCKET socket, int32 backlog)
{
	return SOCKET_ERROR != ::listen(socket, backlog);
}

void SocketUtils::close(SOCKET& socket)
{
	if (socket != INVALID_SOCKET)
		::closesocket(socket);
	socket = INVALID_SOCKET;
}