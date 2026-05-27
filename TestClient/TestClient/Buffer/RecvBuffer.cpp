
#include "RecvBuffer.h"

RecvBuffer::RecvBuffer(int32 BufferSize) : _Buffer_size(BufferSize)
{
	_capacity = BufferSize * 10;
	_buffer.resize(_capacity);
}

RecvBuffer::~RecvBuffer()
{
}

void RecvBuffer::clean()
{
	int32 _dataSize = data_size();
	if (_dataSize == 0)
	{
		// 딱 마침 읽기+쓰기 커서가 동일한 위치라면, 둘 다 리셋.
		_read_pos = _write_pos = 0;
	}
	else
	{
		// 여유 공간이 버퍼 1개 크기 미만이면, 데이터를 앞으로 땅긴다.
		if (free_size() < _Buffer_size)
		{
			::memmove(&_buffer[0], &_buffer[_read_pos], _dataSize);
			_read_pos = 0;
			_write_pos = _dataSize;
		}
	}
}

bool RecvBuffer::on_read(int32 numOfBytes)
{
	if (numOfBytes > data_size())
		return false;

	_read_pos += numOfBytes;
	return true;
}

bool RecvBuffer::on_write(int32 numOfBytes)
{
	if (numOfBytes > free_size())
		return false;

	_write_pos += numOfBytes;
	return true;
}