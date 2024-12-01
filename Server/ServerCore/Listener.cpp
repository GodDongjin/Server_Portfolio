#include "pch.h"
#include "Listener.h"
#include "SocketUtils.h"
#include "Service.h"
#include "Session.h"
#include "SessionManager.h"

Listener::~Listener()
{
	SocketUtils::Close(mSocket);

	for (AcceptEvent* acceptEvent : mAcceptEvents)
	{
		// TODO

		delete(acceptEvent);
	}
}

bool Listener::StartAccept(ServerServiceRef service)
{
	mService = service;
	if (mService == nullptr)
		return false;

	mSocket = SocketUtils::CreateSocket();
	if (mSocket == INVALID_SOCKET)
		return false;

	if (mService->GetIocpCore()->Register(shared_from_this()) == false)
		return false;

	if (SocketUtils::SetReuseAddress(mSocket, true) == false)
		return false;

	if (SocketUtils::SetLinger(mSocket, 0, 0) == false)
		return false;

	if (SocketUtils::Bind(mSocket, mService->GetNetAddress()) == false)
		return false;

	if (SocketUtils::Listen(mSocket) == false)
		return false;

	const int32 acceptCount = mService->GetSessionManager()->GetMaxSessionCount();
	for (int32 i = 0; i < acceptCount; i++)
	{
		AcceptEvent* acceptEvent = new AcceptEvent();
		acceptEvent->SetOwner(shared_from_this());
		mAcceptEvents.push_back(acceptEvent);
		RegisterAccept(acceptEvent);
	}

	return true;
}

void Listener::CloseSocket()
{
	SocketUtils::Close(mSocket);
}

HANDLE Listener::GetHandle()
{
	return reinterpret_cast<HANDLE>(mSocket);
}

void Listener::Dispatch(IocpEvent* iocpEvent, int32 numOfBytes)
{
	ASSERT_CRASH(iocpEvent->GetEventType() == EventType::Accept);
	AcceptEvent* acceptEvent = static_cast<AcceptEvent*>(iocpEvent);
	ProcessAccept(acceptEvent);
}

void Listener::RegisterAccept(AcceptEvent* acceptEvent)
{
	SessionRef session = mService->CreateSession();

	acceptEvent->Init();
	acceptEvent->SetSession(session);

	DWORD bytesReceived = 0;
	if (false == SocketUtils::AcceptEx(mSocket, session->GetSocket(), session->GetRecvBuffer().WritePos(), 0, sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, OUT & bytesReceived, static_cast<LPOVERLAPPED>(acceptEvent)))
	{
		const int32 errorCode = ::WSAGetLastError();
		if (errorCode != WSA_IO_PENDING)
		{
			// 일단 다시 Accept 걸어준다
			RegisterAccept(acceptEvent);
		}
	}
}

void Listener::ProcessAccept(AcceptEvent* acceptEvent)
{
	SessionRef session = acceptEvent->GetSession();

	if (false == SocketUtils::SetUpdateAcceptSocket(session->GetSocket(), mSocket))
	{
		RegisterAccept(acceptEvent);
		return;
	}

	SOCKADDR_IN sockAddress;
	int32 sizeOfSockAddr = sizeof(sockAddress);
	if (SOCKET_ERROR == ::getpeername(session->GetSocket(), OUT reinterpret_cast<SOCKADDR*>(&sockAddress), &sizeOfSockAddr))
	{
		RegisterAccept(acceptEvent);
		return;
	}

	cout << "Success Accept" << endl;

	session->SetNetAddress(NetAddress(sockAddress));
	//session->SetProcessConnect();
	mService->GetSessionManager()->OnConnected(session);
	RegisterAccept(acceptEvent);
}