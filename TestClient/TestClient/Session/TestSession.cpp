#include "TestSession.h"

bool TestSession::connect(const wstring& ip, uint16 port)
{
	WSADATA wsaData;
	if (::WSAStartup(MAKEWORD(2, 2), OUT & wsaData))
	{
		cout << "WSAStartup failed : " << WSAGetLastError() << endl;
		WSACleanup();
		return false;
	}

	SOCKET client_socket = ::WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (client_socket == INVALID_SOCKET)
	{
		cout << "WSASocket failed : " << WSAGetLastError() << endl;
		WSACleanup();
		return false;
	}

	SOCKADDR_IN server_addr;
	::ZeroMemory(&server_addr, sizeof(server_addr));

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = ::htons(port);

	if (::InetPtonW(AF_INET, ip.c_str(), &server_addr.sin_addr) != 1)
	{
		cout << "Invalid ip address : " << ip.c_str() << endl;
		closesocket(client_socket);
		WSACleanup();
		return 0;
	}

	if (::connect(client_socket, reinterpret_cast<SOCKADDR*>(&server_addr), sizeof(server_addr)))
	{
		cout << "connect failed : " << WSAGetLastError() << endl;
		closesocket(client_socket);
		WSACleanup();
		return false;
	}

	_socket = client_socket;

	return true;
}

bool TestSession::register_recv()
{
	WSABUF wsa_buf;
	wsa_buf.buf = reinterpret_cast<char*>(_recv_buffer->write_pos());
	wsa_buf.len = _recv_buffer->free_size();

	DWORD numOfBytes = 0;
	DWORD flags = 0;
	if (SOCKET_ERROR == ::WSARecv(_socket, &wsa_buf, 1, OUT & numOfBytes, OUT & flags, &_recv_overlapped, nullptr))
	{
		int32 errorCode = ::WSAGetLastError();
		if (errorCode != WSA_IO_PENDING)
		{
			cout << "Handle Error : " << errorCode << endl;
			return false;
		}
	}

	return true;
}

bool TestSession::send_packet(const vector<BYTE>& packet)
{
	int32 sent = ::send(
		_socket,
		reinterpret_cast<const char*>(packet.data()),
		static_cast<int>(packet.size()),
		0
	);

	if (sent == SOCKET_ERROR)
	{
		cout << "send failed : " << WSAGetLastError() << endl;
		return false;
	}

	return sent == packet.size();
}

void TestSession::process_send(uint32 num_bytes)
{
}
