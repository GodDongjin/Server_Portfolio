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

	private readonly object _lock = new object(); // lock 객체 추가

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
			// 소켓이 연결되어 있지 않으면 루프 종료
			if (!socket.Connected)
			{
				Debug.LogWarning("Socket is not connected, exiting the loop.");
				break;
			}

			// 수신 패킷 큐에서 패킷을 안전하게 꺼내기
			if (recvPacketQueue.TryDequeue(out buffer))
			{
				lock (_lock) // lock 사용
				{
					packetHandler.OnRecvPacket(this, buffer);
				}
			}
			else
			{
				// 큐가 비어있으면 잠시 대기 (스핀락 방지)
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
