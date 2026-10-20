#include "pch.h"
#include "IocpEvent.h"
#include "SendBuffer.h"

IocpEvent::IocpEvent(EventType type) : _event_type(type)
{
	init();
}

void IocpEvent::init()
{
	OVERLAPPED::hEvent = 0;
	OVERLAPPED::Internal = 0;
	OVERLAPPED::InternalHigh = 0;
	OVERLAPPED::Offset = 0;
	OVERLAPPED::OffsetHigh = 0;
}
void SendEvent::build_wsabufs(vector<WSABUF>& buffers) const
{
	buffers.clear();
	buffers.reserve(_send_Buffers.size() - _buffer_index);
	for (size_t i = _buffer_index; i < _send_Buffers.size(); ++i)
	{
		const auto& buffer = _send_Buffers[i];
		const int32 offset = (i == _buffer_index) ? _buffer_offset : 0;
		WSABUF wsaBuf;
		wsaBuf.buf = reinterpret_cast<char*>(buffer->get_buffer()) + offset;
		wsaBuf.len = static_cast<ULONG>(buffer->get_write_size() - offset);
		buffers.push_back(wsaBuf);
	}
}

bool SendEvent::consume_sent_bytes(int32 numOfBytes)
{
	if (numOfBytes <= 0)
		return false;

	uint64 remaining = 0;
	for (size_t i = _buffer_index; i < _send_Buffers.size(); ++i)
		remaining += _send_Buffers[i]->get_write_size();
	remaining -= _buffer_offset;
	if (static_cast<uint64>(numOfBytes) > remaining)
		return false;

	// 완료 바이트 수가 버퍼 경계를 넘을 수도 있으므로 여러 버퍼에 걸쳐 위치를 갱신한다.
	while (numOfBytes > 0)
	{
		const int32 available = _send_Buffers[_buffer_index]->get_write_size() - _buffer_offset;
		if (numOfBytes < available)
		{
			_buffer_offset += numOfBytes;
			break;
		}
		numOfBytes -= available;
		++_buffer_index;
		_buffer_offset = 0;
	}
	return true;
}
