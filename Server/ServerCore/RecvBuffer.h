#pragma once
class RecvBuffer
{
public:
	RecvBuffer(int32 bufferSize);
	~RecvBuffer();

	void			Clean();
	bool			OnRead(int32 numOfBytes);
	bool			OnWrite(int32 numOfBytes);

	BYTE* ReadPos() { return &mbuffer[mReadPos]; }
	BYTE* WritePos() { return &mbuffer[mWritePos]; }
	int32			DataSize() { return mWritePos - mReadPos; }
	int32			FreeSize() { return mCapacity - mWritePos; }

private:
	int32			mCapacity = 0;
	int32			mbufferSize = 0;
	int32			mReadPos = 0;
	int32			mWritePos = 0;
	vector<BYTE>	mbuffer;
};

