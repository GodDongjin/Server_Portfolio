#pragma once

#include "../Utils/Types.h"
#include "../Buffer/SendBuffer.h"

enum class EventType : uint8
{
	RECV,
	SEND
};

class IocpEvent : public OVERLAPPED
{
public:
	IocpEvent(EventType event_type) : _event_type(event_type)
	{
		init();
	}

	void init()
	{
		OVERLAPPED::hEvent = 0;
		OVERLAPPED::Internal = 0;
		OVERLAPPED::InternalHigh = 0;
		OVERLAPPED::Offset = 0;
		OVERLAPPED::OffsetHigh = 0;
	}

	EventType get_event_type() { return _event_type; }

protected:
	EventType _event_type;
};

class RecvEvent : public IocpEvent
{
public:
	RecvEvent() : IocpEvent(EventType::RECV) {}
};

class SendEvent : public IocpEvent
{
public:
	SendEvent() : IocpEvent(EventType::SEND) {}

	void clear()
	{
		_send_Buffers.clear();
		_buffer_index = 0;
		_buffer_offset = 0;
	}

public:
	const vector<shared_ptr<SendBuffer>> get_send_Buffers() { return _send_Buffers; }
	void send_Buffers_push(shared_ptr<SendBuffer> sendBuffer) { _send_Buffers.push_back(sendBuffer); }
	void send_Buffers_clear() { clear(); }
	bool has_pending_data() const { return _buffer_index < _send_Buffers.size(); }

	void build_wsabufs(vector<WSABUF>& buffers) const
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

	bool consume_sent_bytes(uint32 num_bytes)
	{
		if (num_bytes == 0) {
			return false;
		}

		uint64 remaining = 0;
		for (size_t i = _buffer_index; i < _send_Buffers.size(); ++i) {
			remaining += _send_Buffers[i]->get_write_size();
		}
		remaining -= _buffer_offset;
		if (num_bytes > remaining) {
			return false;
		}

		// 완료된 바이트만 소비하며, 여러 버퍼의 경계를 넘는 경우도 처리한다.
		while (num_bytes > 0)
		{
			const uint32 available = _send_Buffers[_buffer_index]->get_write_size() - _buffer_offset;
			if (num_bytes < available)
			{
				_buffer_offset += static_cast<int32>(num_bytes);
				break;
			}
			num_bytes -= available;
			++_buffer_index;
			_buffer_offset = 0;
		}
		return true;
	}

private:
	// 송신 버퍼를 수정하지 않고 별도 위치로 진행 상황을 추적한다.
	vector<shared_ptr<SendBuffer>> _send_Buffers;
	size_t _buffer_index = 0;
	int32 _buffer_offset = 0;
};
