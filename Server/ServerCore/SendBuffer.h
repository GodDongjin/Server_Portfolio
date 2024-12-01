#pragma once
class SendBuffer : enable_shared_from_this<SendBuffer>
{
public:
	SendBuffer(int32 bufferSize);
	~SendBuffer();

	BYTE* Buffer() { return mBuffer.data(); }
	int32 WriteSize() { return mWriteSize; }
	int32 Capacity() { return static_cast<int32>(mBuffer.size()); }

	void CopyData(void* data, int32 len);
	void Close(uint32 writeSize);

private:
	vector<BYTE>	mBuffer;
	int32			mWriteSize = 0;
};
