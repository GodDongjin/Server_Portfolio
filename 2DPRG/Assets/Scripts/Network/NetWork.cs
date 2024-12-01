using System;
using System.Collections;
using System.Collections.Generic;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Net.Sockets;
using System.Runtime.InteropServices;
using System.Threading;
using Unity.Properties;
using UnityEngine;

public class NetWork
{
	private SendWorker sendWorker;
	private RecvWorker recvWorker;
	private Session session;

	public NetWork(Session _session)
	{
		session = _session;
		recvWorker = new RecvWorker(session);
		sendWorker = new SendWorker(session);
	}

	public void Init()
	{
		Thread RecvWorkerThread = new Thread(recvWorker.Run);
		Thread SendWorkerThread = new Thread(sendWorker.Run);

		RecvWorkerThread.Start();
		SendWorkerThread.Start();
	}
}

public class RecvWorker
{
	public bool Running = true;
	private Session session;
	private byte[] buffer = new byte[1024]; // ���� ����
	private readonly Mutex _mutex = new Mutex();

	public RecvWorker(Session _session)
	{
		session = _session;
	}

	public void Run() // �Ű����� ���� �޼���� ����
	{
		while (Running)
		{
			int numOfByte = 0;
			if (session != null)
			{
				try
				{
					NetworkStream stream = session.GetSocket().GetStream();
					numOfByte = stream.Read(buffer, 0, buffer.Length);

					// �����Ͱ� ���ŵ� ���
					if (numOfByte > 0)
					{
						_mutex.WaitOne(); // Mutex ���
						try
						{
							var recvBuffer = new ArraySegment<byte>(buffer, 0, numOfByte);
							session.recvPacketQueue.Enqueue(recvBuffer);
							session.GetRecvBuffer().OnWrite(numOfByte); // ���� ���ۿ� �ۼ�
						}
						finally
						{
							_mutex.ReleaseMutex(); // Mutex ����
						}
					}
				}
				catch (IOException ioEx)
				{
					Debug.Log($"Network error: {ioEx.Message}");
					Running = false; // ��Ʈ��ũ ���� �߻� �� ����
				}
				catch (Exception ex)
				{
					Debug.Log($"Unexpected error: {ex.Message}");
					Running = false; // ����ġ ���� ���� �߻� �� ����
				}
			}
			else
			{
				Running = false;
			}
		}
	}
}

public class SendWorker
{
	public bool Running = true;
	private readonly Session session;
	private readonly Mutex _mutex = new Mutex();

	public SendWorker(Session _session)
	{
		session = _session;
	}

	public void Run() // �Ű����� ���� �޼���� ����
	{
		while (Running)
		{
			SendBuffer sendBuffer;

			// sendPacketQueue ����
			_mutex.WaitOne(); // Mutex ���
			try
			{
				if (session.sendPacketQueue.TryDequeue(out sendBuffer))
				{
					SendPacket(sendBuffer);
				}
			}
			finally
			{
				_mutex.ReleaseMutex(); // Mutex ����
			}

			if (session == null)
			{
				Debug.LogWarning("Session is null, stopping the run loop.");
				Running = false; // ������ null�� ��� ����
			}
		}
	}

	private void SendPacket(SendBuffer sendBuffer)
	{
		NetworkStream stream = session.GetSocket().GetStream();
		stream.Write(sendBuffer._buffer, 0, sendBuffer._writeSize);
	}
}