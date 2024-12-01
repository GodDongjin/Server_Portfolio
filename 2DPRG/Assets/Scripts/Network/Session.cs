using System;
using System.Collections;
using System.Collections.Generic;
using System.Net.Sockets;
using System.Threading;
using UnityEngine;

public class Session
{
	private TcpClient socket;
	private ClientPacketHandler packetHandler;
	private RecvBuffer recvBuffer;

	public Queue<ArraySegment<byte>> recvPacketQueue;
	public Queue<SendBuffer> sendPacketQueue;

	private UInt64 sessionIdx = 0;

	Thread recvPacketThread;

	private readonly object _lock = new object(); // lock ��ü �߰�

	public void SetSocket(TcpClient _socket) { socket = _socket; }
	public TcpClient GetSocket() { return socket; }
	public ClientPacketHandler GetPacketHandler() { return packetHandler; }
	public RecvBuffer GetRecvBuffer() { return recvBuffer; }
	public void SetSessionIdx(UInt64 idx) { sessionIdx = idx; }
	public UInt64 GetSessionIdx() { return sessionIdx; }

	public Session() 
	{ 
		packetHandler = new ClientPacketHandler();
		recvBuffer = new RecvBuffer(8192);
		sendPacketQueue = new Queue<SendBuffer>();
		recvPacketQueue = new Queue<ArraySegment<byte>>();
		recvPacketThread = new Thread(RecvPacket);
		packetHandler.Init(); 
	}

	public void RecvPacketStart()
	{
		recvPacketThread.Start();
	}

	public void RecvPacket()
	{
		ArraySegment<byte> buffer;
		while (true)
		{
			// ������ ����Ǿ� ���� ������ ���� ����
			if (!socket.Connected)
			{
				Debug.LogWarning("Socket is not connected, exiting the loop.");
				break;
			}

			// ���� ��Ŷ ť���� ��Ŷ�� �����ϰ� ������
			if (recvPacketQueue.TryDequeue(out buffer))
			{
				lock (_lock) // lock ���
				{
					packetHandler.OnRecvPacket(this, buffer);
				}
			}
			else
			{
				// ť�� ��������� ��� ��� (���ɶ� ����)
				Thread.Sleep(1);
			}
		}
	}

	public void SendPacket(SendBuffer _sendBuffer)
	{
		if (_sendBuffer == null)
			return;

		sendPacketQueue.Enqueue(_sendBuffer);
	}

}
