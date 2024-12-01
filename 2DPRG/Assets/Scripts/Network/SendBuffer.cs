using System;
using System.Collections;
using System.Collections.Generic;
using Unity.Burst;
using UnityEngine;
//using static TMPro.SpriteAssetUtilities.TexturePacker_JsonArray;

public class SendBuffer
{
    public byte[] _buffer;
    public int _writeSize = 0;

    public int FreeSize { get { return _buffer.Length - _writeSize; } }
    public SendBuffer(int bufferSize)
    {
        _buffer = new byte[bufferSize];
	}

	// SendBuffer에 데이터를 담은 후 실행
	public void Close(int writeSize)
	{
		_writeSize = writeSize;
	}
}
