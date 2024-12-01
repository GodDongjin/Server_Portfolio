#include "pch.h"
#include "SendBuffer.h"

SendBuffer::SendBuffer(int32 bufferSize)
{
	mBuffer.resize(bufferSize);
}

SendBuffer::~SendBuffer()
{
}

void SendBuffer::CopyData(void* data, int32 len)
{
	assert(Capacity() >= len);
	::memcpy(mBuffer.data(), data, len);
	mWriteSize = len;
}

void SendBuffer::Close(uint32 writeSize)
{
	mWriteSize = writeSize;
}

