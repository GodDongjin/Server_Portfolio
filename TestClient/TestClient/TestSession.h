#pragma once
#include <windows.h>
#include <iostream>

#include <winsock2.h>
#include <mswsock.h>
#include <ws2tcpip.h>

#include <vector>

using namespace std;

class TestSession
{
public:
	bool connect(const wstring& ip, uint16 prot)

private:
	SOCKET _socket;

	WSAOVERLAPPED _recv_overlapped = {};
	WSAOVERLAPPED _send_overlapped = {};

	WSABUF _recv_wsa_buf = {};
	WSABUF _send_wsa_buf = {};
	
	vector<BYTE> _recv_buffer;
	vector<BYTE> _send_buffer;
};