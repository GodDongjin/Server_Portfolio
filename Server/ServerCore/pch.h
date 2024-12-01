#pragma once

#define WIN32_LEAN_AND_MEAN // 거의 사용되지 않는 내용을 Windows 헤더에서 제외합니다.

#include "CorePch.h"


#define SEND_PACKET(pkt)												\
	SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(pkt); \
	session->Send(sendBuffer);