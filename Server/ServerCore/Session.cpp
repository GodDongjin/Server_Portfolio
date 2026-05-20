#include "pch.h"
#include "Session.h"
#include "SocketUtils.h"
#include "SendBuffer.h"
//#include "ServerPacketHandler.h"

Session::Session() : _recv_buffer(BUFFER_SIZE)
{
	_socket = SocketUtils::create_socket();
}

Session::~Session()
{
	SocketUtils::close(_socket);
}

void Session::send(SendBufferRef sendBuffer)
{
	if (is_connected() == false)
		return;

	bool registerSend = false;

	{
		WRITE_LOCK;

		_send_queue.push(sendBuffer);

		if (_is_send_register.exchange(true) == false)
			registerSend = true;

		if (registerSend)
			register_send();
	}
}

void Session::disconnect(const WCHAR* cause)
{
	if (_is_connect.exchange(false) == false)
		return;

	// TEMP
	wcout << "disconnect : " << cause << endl;

	register_disconnect();
}

HANDLE Session::get_handle()
{
	return reinterpret_cast<HANDLE>(_socket);
}

void Session::dispatch(IocpEvent* iocpEvent, int32 numOfBytes)
{
	switch (iocpEvent->get_event_type())
	{
	case EventType::connect:
		process_connect();
		break;
	case EventType::disconnect:
		process_disconnect();
		break;
	case EventType::Recv:
		process_recv(numOfBytes);
		break;
	case EventType::send:
		process_send(numOfBytes);
		break;
	default:
		break;
	}
}

//bool Session::RegisterConnect()
//{
//	if (is_connected())
//		return false;
//
//	if (GetService()->get_service_type() != ServiceType::Client)
//		return false;
//
//	if (SocketUtils::set_resuse_address(_socket, true) == false)
//		return false;
//
//	if (SocketUtils::bind_any_address(_socket, 0) == false)
//		return false;
//
//	_connect_event.init();
//	_connect_event.set_owner(shared_from_this());
//
//	DWORD numOfBytes = 0;
//	SOCKADDR_IN sockAddr = GetService()->get_net_address().get_sock_addr();
//	if (false == SocketUtils::ConnectEx(_socket, reinterpret_cast<SOCKADDR*>(&sockAddr), sizeof(sockAddr), nullptr, 0, &numOfBytes, &_connect_event))
//	{
//		int32 errorCode = ::WSAGetLastError();
//		if (errorCode != WSA_IO_PENDING)
//		{
//			_connect_event.set_owner(nullptr); 
//			return false;
//		}
//	}
//
//	return true;
//}

bool Session::register_disconnect()
{
	_disconnect_event.init();
	_disconnect_event.set_owner(shared_from_this());

	if (false == SocketUtils::DisconnectEx(_socket, &_disconnect_event, TF_REUSE_SOCKET, 0))
	{
		int32 errorCode = ::WSAGetLastError();
		if (errorCode != WSA_IO_PENDING)
		{
			_disconnect_event.set_owner(nullptr);
			return false;
		}
	}

	return true;
}

void Session::register_recv()
{
	if (is_connected() == false)
		return;

	_recv_event.init();
	_recv_event.set_owner(shared_from_this());

	WSABUF wsaBuf;
	wsaBuf.buf = reinterpret_cast<char*>(_recv_buffer.write_pos());
	wsaBuf.len = _recv_buffer.free_size();

	DWORD numOfBytes = 0;
	DWORD flags = 0;
	if (SOCKET_ERROR == ::WSARecv(_socket, &wsaBuf, 1, OUT & numOfBytes, OUT & flags, &_recv_event, nullptr))
	{
		int32 errorCode = ::WSAGetLastError();
		if (errorCode != WSA_IO_PENDING)
		{
			handle_error(errorCode);
			_recv_event.set_owner(nullptr);
		}
	}
}

void Session::register_send()
{
	if (is_connected() == false)
		return;

	_send_event.init();
	_send_event.set_owner(shared_from_this());

	{
		int32 writeSize = 0;
		while (_send_queue.empty() == false)
		{
			SendBufferRef sendBuffer = _send_queue.front();

			writeSize += sendBuffer->get_write_size();
			// TODO : 예외 체크

			_send_queue.pop();
			_send_event.send_Buffers_push(sendBuffer);
		}
	}

	vector<WSABUF> wsaBufs;
	wsaBufs.reserve(_send_event.get_send_Buffers().size());
	for (SendBufferRef sendBuffer : _send_event.get_send_Buffers())
	{
		WSABUF wsaBuf;
		wsaBuf.buf = reinterpret_cast<char*>(sendBuffer->get_buffer());
		wsaBuf.len = static_cast<LONG>(sendBuffer->get_write_size());
		wsaBufs.push_back(wsaBuf);
	}

	DWORD numOfBytes = 0;
	if (SOCKET_ERROR == ::WSASend(_socket, wsaBufs.data(), static_cast<DWORD>(wsaBufs.size()), OUT & numOfBytes, 0, &_send_event, nullptr))
	{
		int32 errorCode = ::WSAGetLastError();
		if (errorCode != WSA_IO_PENDING)
		{
			handle_error(errorCode);
			_send_event.set_owner(nullptr);
			_send_event.send_Buffers_clear();
			_is_send_register.store(false);
		}
	}
}

void Session::process_connect()
{
	_connect_event.set_owner(nullptr);

	_is_connect.store(true);

	//on_connected();

	register_recv();
}

void Session::process_disconnect()
{
	_disconnect_event.set_owner(nullptr);
}

void Session::process_recv(int32 numOfBytes)
{
	_recv_event.set_owner(nullptr);

	if (numOfBytes == 0)
	{
		disconnect(L"Recv 0");
		return;
	}

	if (_recv_buffer.on_write(numOfBytes) == false)
	{
		disconnect(L"on_write Overflow");
		return;
	}

	int32 dataSize = _recv_buffer.data_size();
	int32 processLen = on_recv(_recv_buffer.read_pos(), dataSize); 
	if (processLen < 0 || dataSize < processLen || _recv_buffer.on_read(processLen) == false)
	{
		disconnect(L"on_read Overflow");
		return;
	}

	_recv_buffer.clean();

	register_recv();
}

void Session::process_send(int32 numOfBytes)
{
	_send_event.set_owner(nullptr);
	_send_event.send_Buffers_clear();

	if (numOfBytes == 0)
	{
		disconnect(L"send 0");
		return;
	}

	on_send(numOfBytes);

	WRITE_LOCK;
	if (_send_queue.empty())
		_is_send_register.store(false);
	else
		register_send();
}

void Session::handle_error(int32 errorCode)
{
	switch (errorCode)
	{
	case WSAECONNRESET:
	case WSAECONNABORTED:
		disconnect(L"handle_error");
		break;
	default:
		// TODO : Log
		cout << "Handle Error : " << errorCode << endl;
		break;
	}
}

int32 Session::on_recv(BYTE* get_buffer, int32 len)
{
	int32 processLen = 0;

	while (true)
	{
		int32 dataSize = len - processLen;
		// 최소한 헤더는 파싱할 수 있어야 한다
		if (dataSize < sizeof(PacketHeader))
			break;

		PacketHeader header = *(reinterpret_cast<PacketHeader*>(&get_buffer[processLen]));
		// 헤더에 기록된 패킷 크기를 파싱할 수 있어야 한다
		if (dataSize < header.size)
			break;

		// 패킷 조립 성공
		on_recv_packet(&get_buffer[processLen], header.size);

		processLen += header.size;
	}

	return processLen;
}


void Session::on_recv_packet(BYTE* get_buffer, int32 len)
{
	
}