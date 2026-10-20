#include "pch.h"
#include "Session.h"
#include "SocketUtils.h"
#include "SendBuffer.h"
#include "Service.h"
#include "../Job/JobQueue.h"
#include <utility>

Session::Session() : _logic_queue(make_shared<JobQueue>()), _recv_buffer(BUFFER_SIZE)
{
	_socket = SocketUtils::create_socket();
}

Session::~Session()
{
	SocketUtils::close(_socket);
}

void Session::post_logic(std::function<void()> callback)
{
	// pushOnly를 true로 전달해 호출한 자리에서 실행하지 않고 큐에 등록한다.
	// 작업 도중 disconnect가 호출되어도 종료 정리는 현재 작업이 끝난 뒤 실행된다.
	// 종료 정리도 등록해야 하므로 여기서는 연결이 끊긴 세션의 작업을 거부하지 않는다.
	_logic_queue->push(make_shared<Job>(std::move(callback)), true);
}

void Session::send(SendBufferRef sendBuffer)
{
	if (is_connected() == false)
		return;

	bool registerSend = false;
	bool needDisconnect = false;

	{
		WRITE_LOCK;

		// 송신 큐가 무한히 증가하는 상황을 방지하기 위해 _send_queue.size 제한 및 체크.
		if (_send_queue.size() >= MAX_SEND_QUEUE_SIZE)
		{
			needDisconnect = true;
		}
		else
		{
			_send_queue.push(sendBuffer);

			if (_is_send_register.exchange(true) == false) {
				registerSend = true;
			}	
		}
	}

	if (needDisconnect)
	{
		ERROR_LOG("send queue overflow");
		disconnect();
		return;
	}

	if (registerSend) {
		register_send();
	}
}

void Session::reject_connection(int32 max_session_count)
{
	if (_is_connect.exchange(true))
		return;

	// 정상 수신은 시작하지 않는다. 송신 이벤트의 shared_ptr가 완료까지 세션을 유지한다.
	_close_after_send.store(true);
	GServerStats.connect++;
	SendBufferRef notice = make_server_full_notice(max_session_count);
	if (notice == nullptr)
	{
		disconnect();
		return;
	}

	send(notice);
}

void Session::disconnect()
{
	if (_is_connect.exchange(false) == false) {
		return;
	}

	if (register_disconnect() == false) {
		process_disconnect();
	}
	
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
			// 등록 실패에는 완료 통지가 오지 않으므로 참조를 먼저 해제한다.
			_recv_event.set_owner(nullptr);
			handle_error(errorCode);
		}
	}
}

void Session::register_send()
{
	// 이미 등록된 비동기 송신 버퍼는 그 완료 통지를 받을 때까지 유지한다.
	if (is_connected() == false) {
		return;
	}

	_send_event.init();
	_send_event.set_owner(shared_from_this());

	// 부분 송신이면 기존 버퍼와 진행 위치를 유지하고 남은 구간부터 이어 보낸다.
	if (!_send_event.has_pending_data())
	{
		WRITE_LOCK;
		while (!_send_queue.empty())
		{
			_send_event.send_Buffers_push(_send_queue.front());
			_send_queue.pop();
		}
	}

	vector<WSABUF> wsaBufs;
	_send_event.build_wsabufs(wsaBufs);
	if (wsaBufs.empty())
	{
		_send_event.set_owner(nullptr);
		_send_event.send_Buffers_clear();
		_is_send_register.store(false);
		if (_close_after_send.load())
			disconnect();
		return;
	}

	if (SOCKET_ERROR == ::WSASend(_socket, wsaBufs.data(), static_cast<DWORD>(wsaBufs.size()), nullptr, 0, &_send_event, nullptr))
	{
		int32 errorCode = ::WSAGetLastError();
		if (errorCode != WSA_IO_PENDING)
		{
			// 즉시 실패하면 완료 통지가 오지 않으므로 여기서 참조를 해제하고 종료한다.
			_send_event.set_owner(nullptr);
			_send_event.send_Buffers_clear();
			_is_send_register.store(false);
			ERROR_LOG("WSASend failed");
			disconnect();
		}
	}
}

void Session::process_connect()
{
	_connect_event.set_owner(nullptr);
	_is_connect.store(true);

	GServerStats.connect++;

	register_recv();
}

void Session::process_disconnect()
{
	_disconnect_event.set_owner(nullptr);

	if (_is_disconnect.exchange(true))
		return;

	// 연결 상태를 먼저 바꿔 아직 실행되지 않은 패킷 작업이 건너뛰도록 한다.
	_is_connect.store(false);
	_is_send_register.store(false);

	// shared_ptr를 값으로 캡처해 실행 중인 핸들러와 종료 정리가 끝날 때까지 세션을 유지한다.
	// 방 퇴장, 로그아웃, 세션 등록 해제까지 같은 큐에서 순서대로 처리한다.
	SessionRef self = get_session();
	post_logic([self]()
	{
		self->on_disconnect();

		auto service = self->get_service().lock();
		if (service){
			service->get_sessionManager()->on_disconnected(self);
		}
			

		// 등록 해제에는 기존 계정 ID와 이름이 필요하므로 정리가 끝난 후 초기화한다.
		self->set_account_idx(0);
		self->set_name(L"");
		self->set_room_id(-1);
	});
}

void Session::process_recv(int32 numOfBytes)
{
	_recv_event.set_owner(nullptr);

	if (is_connected() == false) {
		return;
	}		

	if (numOfBytes == 0)
	{
		disconnect();
		ERROR_LOG("disconnect : Recv 0");
		return;
	}

	if (_recv_buffer.on_write(numOfBytes) == false)
	{
		disconnect();
		ERROR_LOG("disconnect : on_write Overflow");
		return;
	}

	int32 dataSize = _recv_buffer.data_size();
	int32 processLen = on_recv(_recv_buffer.read_pos(), dataSize); 
	if (processLen < 0 || dataSize < processLen || _recv_buffer.on_read(processLen) == false)
	{
		disconnect();
		ERROR_LOG("disconnect : on_read Overflow");
		return;
	}

	_recv_buffer.clean();

	register_recv();
}

void Session::process_send(int32 numOfBytes)
{
	_send_event.set_owner(nullptr);

	if (is_connected() == false)
	{
		_send_event.send_Buffers_clear();
		return;
	}

	if (!_send_event.consume_sent_bytes(numOfBytes))
	{
		_send_event.send_Buffers_clear();
		_is_send_register.store(false);
		disconnect();
		ERROR_LOG("disconnect : invalid send completion");
		return;
	}

	on_send(numOfBytes);

	if (_send_event.has_pending_data())
	{
		register_send();
		return;
	}

	_send_event.send_Buffers_clear();
	if (_close_after_send.load())
	{
		// 안내 패킷의 마지막 바이트까지 송신 완료된 뒤에만 연결을 끊는다.
		_is_send_register.store(false);
		disconnect();
		return;
	}

	bool need_register = false;
	{
		WRITE_LOCK;
		if (_send_queue.empty())
			_is_send_register.store(false);
		else
			need_register = true;
	}

	if (need_register)
		register_send();
}

void Session::handle_error(int32 errorCode)
{
	// WSA_IO_PENDING은 호출부에서 제외한다. 나머지 수신 등록 실패는 연결을 정리한다.
	// 로그만 남기면 수신이 멈춘 세션이 로그인 상태와 접속 자리를 계속 차지한다.
	ERROR_LOG("WSARecv failed: " + to_string(errorCode));
	disconnect();
}

void Session::on_disconnect()
{

}

int32 Session::on_recv(BYTE* get_buffer, int32 len)
{
	int32 processLen = 0;

	while (true)
	{
		int32 dataSize = len - processLen;
		
		// 받은 패킷 크기가 header보다 작으면 문제.
		if (dataSize < sizeof(PacketHeader))
			break;

		PacketHeader header = *(reinterpret_cast<PacketHeader*>(&get_buffer[processLen]));

		// 패킷 사이즈 채크.
		if (header.size < sizeof(PacketHeader))
			return -1;

		if (header.size > BUFFER_SIZE)
			return -1;

		if (dataSize < header.size)
			break;


		// 패킷 처리 작업 진행.
		on_recv_packet(&get_buffer[processLen], header.size);

		processLen += header.size;

		if (!is_connected())
		{
			break;
		}
	}

	return processLen;
}


void Session::on_recv_packet(BYTE* get_buffer, int32 len)
{
	
}
