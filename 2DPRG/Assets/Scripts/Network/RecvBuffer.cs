using System;
using System.Collections;
using System.Collections.Generic;
using System.Numerics;

public class RecvBuffer
{
    ArraySegment<byte> _buffer;
	int _readPos = 0;
	int _writePos = 0;
    
    public RecvBuffer(int bufferSize)
    {
        _buffer = new ArraySegment<byte>(new byte[bufferSize], 0, bufferSize);
	}

	public ArraySegment<byte> ReadPos() { return new ArraySegment<byte>(_buffer.Array, _buffer.Offset + _readPos, DataSize); }
	public ArraySegment<byte> WritePos() { return new ArraySegment<byte>(_buffer.Array, _buffer.Offset + _writePos, FreeSize); ; }
	public int DataSize { get { return _writePos - _readPos; } }
	public int FreeSize { get { return _buffer.Count - _writePos; } }

	public void Clean()
    {
		int dataSize = DataSize;

        if(dataSize ==  0)
        {
            _readPos = _writePos = 0;
        }
        else
        {
            Array.Copy(_buffer.Array, _buffer.Offset + _readPos, _buffer.Array, _buffer.Offset, DataSize);
            _readPos = 0;
            _writePos = dataSize;
        }
    }

	public bool OnRead(int numOfBytes)
    {
        if (numOfBytes > DataSize)
            return false;

        _readPos += numOfBytes; 
        return true;
    }

    public bool OnWrite(int numOfBytes)
    {
        if (numOfBytes > FreeSize)
            return false;

        _writePos += numOfBytes;
        return true;
    }
}
