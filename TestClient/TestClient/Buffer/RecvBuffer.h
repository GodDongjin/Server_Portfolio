#pragma once
#include "../Utils/Types.h"

class RecvBuffer
{
public:
	RecvBuffer(int32 BufferSize);
	~RecvBuffer();

	void clean();
	bool on_read(int32 numOfBytes);
	bool on_write(int32 numOfBytes);

	BYTE* read_pos() { return &_buffer[_read_pos]; }
	BYTE* write_pos() { return &_buffer[_write_pos]; }
	int32 data_size() { return _write_pos - _read_pos; }
	int32 free_size() { return _capacity - _write_pos; }

private:
	int32			_capacity = 0;
	int32			_Buffer_size = 0;
	int32			_read_pos = 0;
	int32			_write_pos = 0;
	vector<BYTE>	_buffer;
};

