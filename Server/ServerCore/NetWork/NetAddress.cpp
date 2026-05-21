#include "pch.h"
#include "NetAddress.h"

NetAddress::NetAddress(SOCKADDR_IN sockAddr) : _sock_addr(sockAddr)
{
}

NetAddress::NetAddress(wstring ip, uint16 port)
{
	::memset(&_sock_addr, 0, sizeof(_sock_addr));
	_sock_addr.sin_family = AF_INET;
	_sock_addr.sin_addr = ip_to_address(ip.c_str());
	_sock_addr.sin_port = ::htons(port);
}

wstring NetAddress::get_ip_address()
{
	WCHAR get_buffer[100];
	::InetNtopW(AF_INET, &_sock_addr.sin_addr, get_buffer, len32(get_buffer));
	return wstring(get_buffer);
}

IN_ADDR NetAddress::ip_to_address(const WCHAR* ip)
{
	IN_ADDR address;
	::InetPtonW(AF_INET, ip, &address);
	return address;
}
