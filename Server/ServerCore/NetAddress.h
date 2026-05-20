#pragma once

class NetAddress
{
public:
	NetAddress() = default;
	NetAddress(SOCKADDR_IN sockAddr);
	NetAddress(wstring ip, uint16 port);

	SOCKADDR_IN&	get_sock_addr() { return _sock_addr; }
	wstring			get_ip_address();
	uint16			get_port() { return ::ntohs(_sock_addr.sin_port); }

public:
	static IN_ADDR	ip_to_address(const WCHAR* ip);

private:
	SOCKADDR_IN		_sock_addr = {};
};

