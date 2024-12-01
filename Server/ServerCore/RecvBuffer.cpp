#include "pch.h"
#include "RecvBuffer.h"

RecvBuffer::RecvBuffer(int32 bufferSize) : mbufferSize(bufferSize)
{
	mCapacity = bufferSize * 10;
	mbuffer.resize(mCapacity);
}

RecvBuffer::~RecvBuffer()
{
}

void RecvBuffer::Clean()
{
	int32 dataSize = DataSize();
	if (dataSize == 0)
	{
		// �� ��ħ �б�+���� Ŀ���� ������ ��ġ���, �� �� ����.
		mReadPos = mWritePos = 0;
	}
	else
	{
		// ���� ������ ���� 1�� ũ�� �̸��̸�, �����͸� ������ �����.
		if (FreeSize() < mbufferSize)
		{
			::memcpy(&mbuffer[0], &mbuffer[mReadPos], dataSize);
			mReadPos = 0;
			mWritePos = dataSize;
		}
	}
}

bool RecvBuffer::OnRead(int32 numOfBytes)
{
	if (numOfBytes > DataSize())
		return false;

	mReadPos += numOfBytes;
	return true;
}

bool RecvBuffer::OnWrite(int32 numOfBytes)
{
	if (numOfBytes > FreeSize())
		return false;

	mWritePos += numOfBytes;
	return true;
}