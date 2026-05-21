#include "SendBuffer.h"

SendBuffer::SendBuffer(int32 BufferSize)
{
	_buffer.resize(BufferSize);
}

SendBuffer::~SendBuffer()
{
}

void SendBuffer::copy_data(void* data, int32 len)
{
	assert(capacity() >= len);
	::memcpy(_buffer.data(), data, len);
	_write_size = len;
}

void SendBuffer::close(uint32 writeSize)
{
	_write_size = writeSize;
}

