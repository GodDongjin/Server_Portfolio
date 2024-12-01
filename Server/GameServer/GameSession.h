#pragma once
#include "Session.h"

class GameSession : public Session
{
public:
	~GameSession()
	{
		cout << "~GameSession" << endl;
	}

public:
	virtual void		OnSend(int32 len) override;
	virtual void		OnRecvPacket(BYTE* buffer, int32 len) override;
};

