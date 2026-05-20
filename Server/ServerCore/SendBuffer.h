#pragma once
class SendBuffer : enable_shared_from_this<SendBuffer>
{
public:
	SendBuffer(int32 BufferSize);
	~SendBuffer();

	BYTE* get_buffer() { return _buffer.data(); }
	int32 get_write_size() { return _write_size; }
	int32 capacity() { return static_cast<int32>(_buffer.size()); }

	void copy_data(void* data, int32 len);
	void close(uint32 writeSize);

private:
	vector<BYTE>	_buffer;
	int32			_write_size = 0;
};
