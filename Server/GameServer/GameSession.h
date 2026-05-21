#pragma once
#include "../ServerCore/NetWork/Session.h"

class GameSession : public Session
{
public:
	~GameSession()
	{
		cout << "~GameSession" << endl;
	}

public:
	virtual void		on_send(int32 len) override;
	virtual void		on_recv_packet(BYTE* buffer, int32 len) override;
};

