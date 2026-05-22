#include "TestSession.h"
#include "../Protocol/Protocol.pb.h"
#include "../Packet/ClientPacketHandler.h"

bool TestSession::connect(SOCKADDR_IN server_addr)
{
	SOCKET client_socket = ::WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (client_socket == INVALID_SOCKET)
	{
		cout << "WSASocket failed : " << WSAGetLastError() << endl;
		WSACleanup();
		return NULL;
	}

	if (::connect(client_socket, reinterpret_cast<SOCKADDR*>(&server_addr), sizeof(server_addr)))
	{
		cout << "connect failed : " << WSAGetLastError() << endl;
		closesocket(client_socket);
		WSACleanup();
		return false;
	}

	_socket = client_socket;
	_is_connect.store(true);

	cout << "connect succes" << endl;

	send();

	return true;
}

void TestSession::send(/*shared_ptr<SendBuffer> send_buffer*/)
{
	if (is_connected() == false)
		return;
	
	//임시 코드
	shared_ptr<SendBuffer> temp;
	{
		Protocol::C_CHAR chat_pkt;
		chat_pkt.set_message("test1");

		temp = ClientPacketHandler::MakeSendBuffer(chat_pkt, PKT_C_CHAR);
		_send_buffer = temp;
	}


	//bool register_send = false;

	//register_send(); 함수로 빼야함.
	{
		WSABUF wsa_buf;
		wsa_buf.buf = reinterpret_cast<char*>(_send_buffer->get_buffer());
		wsa_buf.len = static_cast<LONG>(_send_buffer->get_write_size());

		DWORD numOfBytes = 0;
		if (SOCKET_ERROR == ::WSASend(_socket, &wsa_buf, 1, OUT & numOfBytes, 0, &_send_overlapped, nullptr))
		{
			int32 errorCode = ::WSAGetLastError();
			if (errorCode != WSA_IO_PENDING)
			{
				cout << "WSASend Error : " << errorCode << endl;
			}
		}
	}

	register_recv();
}

void TestSession::dispatch(IocpEvent* iocp_evnet)
{
	switch (iocp_evnet->get_event_type())
	{
	case EventType::SEND:
		break;
	case EventType::RECV:
		break;

	default:
		break;
	}

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

	cout << "register_recv succes" << endl;

	return true;
}

bool TestSession::register_send()
{
	return false;
}

void TestSession::process_send(uint32 num_bytes)
{
}
