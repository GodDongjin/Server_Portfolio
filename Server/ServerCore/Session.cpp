#include "pch.h"
#include "Session.h"
#include "SocketUtils.h"
#include "SendBuffer.h"
//#include "ServerPacketHandler.h"

Session::Session() : mRecvBuffer(BUFFER_SIZE)
{
	mSocket = SocketUtils::CreateSocket();
}

Session::~Session()
{
	SocketUtils::Close(mSocket);
}

void Session::Send(SendBufferRef sendBuffer)
{
	if (IsConnected() == false)
		return;

	bool registerSend = false;

	{
		WRITE_LOCK;

		mSendQueue.push(sendBuffer);

		if (mSendRegistered.exchange(true) == false)
			registerSend = true;

		if (registerSend)
			RegisterSend();
	}
}

void Session::Disconnect(const WCHAR* cause)
{
	if (mConnected.exchange(false) == false)
		return;

	// TEMP
	wcout << "Disconnect : " << cause << endl;

	RegisterDisconnect();
}

HANDLE Session::GetHandle()
{
	return reinterpret_cast<HANDLE>(mSocket);
}

void Session::Dispatch(IocpEvent* iocpEvent, int32 numOfBytes)
{
	switch (iocpEvent->GetEventType())
	{
	case EventType::Connect:
		ProcessConnect();
		break;
	case EventType::Disconnect:
		ProcessDisconnect();
		break;
	case EventType::Recv:
		ProcessRecv(numOfBytes);
		break;
	case EventType::Send:
		ProcessSend(numOfBytes);
		break;
	default:
		break;
	}
}

//bool Session::RegisterConnect()
//{
//	if (IsConnected())
//		return false;
//
//	if (GetService()->GetServiceType() != ServiceType::Client)
//		return false;
//
//	if (SocketUtils::SetReuseAddress(mSocket, true) == false)
//		return false;
//
//	if (SocketUtils::BindAnyAddress(mSocket, 0) == false)
//		return false;
//
//	mConnectEvent.Init();
//	mConnectEvent.SetOwner(shared_from_this());
//
//	DWORD numOfBytes = 0;
//	SOCKADDR_IN sockAddr = GetService()->GetNetAddress().GetSockAddr();
//	if (false == SocketUtils::ConnectEx(mSocket, reinterpret_cast<SOCKADDR*>(&sockAddr), sizeof(sockAddr), nullptr, 0, &numOfBytes, &mConnectEvent))
//	{
//		int32 errorCode = ::WSAGetLastError();
//		if (errorCode != WSA_IO_PENDING)
//		{
//			mConnectEvent.SetOwner(nullptr); 
//			return false;
//		}
//	}
//
//	return true;
//}

bool Session::RegisterDisconnect()
{
	mDisconnectEvent.Init();
	mDisconnectEvent.SetOwner(shared_from_this());

	if (false == SocketUtils::DisconnectEx(mSocket, &mDisconnectEvent, TF_REUSE_SOCKET, 0))
	{
		int32 errorCode = ::WSAGetLastError();
		if (errorCode != WSA_IO_PENDING)
		{
			mDisconnectEvent.SetOwner(nullptr);
			return false;
		}
	}

	return true;
}

void Session::RegisterRecv()
{
	if (IsConnected() == false)
		return;

	mRecvEvent.Init();
	mRecvEvent.SetOwner(shared_from_this());

	WSABUF wsaBuf;
	wsaBuf.buf = reinterpret_cast<char*>(mRecvBuffer.WritePos());
	wsaBuf.len = mRecvBuffer.FreeSize();

	DWORD numOfBytes = 0;
	DWORD flags = 0;
	if (SOCKET_ERROR == ::WSARecv(mSocket, &wsaBuf, 1, OUT & numOfBytes, OUT & flags, &mRecvEvent, nullptr))
	{
		int32 errorCode = ::WSAGetLastError();
		if (errorCode != WSA_IO_PENDING)
		{
			HandleError(errorCode);
			mRecvEvent.SetOwner(nullptr);
		}
	}
}

void Session::RegisterSend()
{
	if (IsConnected() == false)
		return;

	mSendEvent.Init();
	mSendEvent.SetOwner(shared_from_this());

	{
		int32 writeSize = 0;
		while (mSendQueue.empty() == false)
		{
			SendBufferRef sendBuffer = mSendQueue.front();

			writeSize += sendBuffer->WriteSize();
			// TODO : 예외 체크

			mSendQueue.pop();
			mSendEvent.SendBuffersPush(sendBuffer);
		}
	}

	vector<WSABUF> wsaBufs;
	wsaBufs.reserve(mSendEvent.GetSendBuffer().size());
	for (SendBufferRef sendBuffer : mSendEvent.GetSendBuffer())
	{
		WSABUF wsaBuf;
		wsaBuf.buf = reinterpret_cast<char*>(sendBuffer->Buffer());
		wsaBuf.len = static_cast<LONG>(sendBuffer->WriteSize());
		wsaBufs.push_back(wsaBuf);
	}

	DWORD numOfBytes = 0;
	if (SOCKET_ERROR == ::WSASend(mSocket, wsaBufs.data(), static_cast<DWORD>(wsaBufs.size()), OUT & numOfBytes, 0, &mSendEvent, nullptr))
	{
		int32 errorCode = ::WSAGetLastError();
		if (errorCode != WSA_IO_PENDING)
		{
			HandleError(errorCode);
			mSendEvent.SetOwner(nullptr);
			mSendEvent.SendBuffersClear();
			mSendRegistered.store(false);
		}
	}
}

void Session::ProcessConnect()
{
	mConnectEvent.SetOwner(nullptr);

	mConnected.store(true);

	//OnConnected();

	RegisterRecv();
}

void Session::ProcessDisconnect()
{
	mDisconnectEvent.SetOwner(nullptr);
}

void Session::ProcessRecv(int32 numOfBytes)
{
	mRecvEvent.SetOwner(nullptr);

	if (numOfBytes == 0)
	{
		Disconnect(L"Recv 0");
		return;
	}

	if (mRecvBuffer.OnWrite(numOfBytes) == false)
	{
		Disconnect(L"OnWrite Overflow");
		return;
	}

	int32 dataSize = mRecvBuffer.DataSize();
	int32 processLen = OnRecv(mRecvBuffer.ReadPos(), dataSize); 
	if (processLen < 0 || dataSize < processLen || mRecvBuffer.OnRead(processLen) == false)
	{
		Disconnect(L"OnRead Overflow");
		return;
	}

	mRecvBuffer.Clean();

	RegisterRecv();
}

void Session::ProcessSend(int32 numOfBytes)
{
	mSendEvent.SetOwner(nullptr);
	mSendEvent.SendBuffersClear();

	if (numOfBytes == 0)
	{
		Disconnect(L"Send 0");
		return;
	}

	OnSend(numOfBytes);

	WRITE_LOCK;
	if (mSendQueue.empty())
		mSendRegistered.store(false);
	else
		RegisterSend();
}

void Session::HandleError(int32 errorCode)
{
	switch (errorCode)
	{
	case WSAECONNRESET:
	case WSAECONNABORTED:
		Disconnect(L"HandleError");
		break;
	default:
		// TODO : Log
		cout << "Handle Error : " << errorCode << endl;
		break;
	}
}

int32 Session::OnRecv(BYTE* buffer, int32 len)
{
	int32 processLen = 0;

	while (true)
	{
		int32 dataSize = len - processLen;
		// 최소한 헤더는 파싱할 수 있어야 한다
		if (dataSize < sizeof(PacketHeader))
			break;

		PacketHeader header = *(reinterpret_cast<PacketHeader*>(&buffer[processLen]));
		// 헤더에 기록된 패킷 크기를 파싱할 수 있어야 한다
		if (dataSize < header.size)
			break;

		// 패킷 조립 성공
		OnRecvPacket(&buffer[processLen], header.size);

		processLen += header.size;
	}

	return processLen;
}


void Session::OnRecvPacket(BYTE* buffer, int32 len)
{
	
}