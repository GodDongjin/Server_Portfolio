#pragma once
#include "../ServerCore/NetWork/Session.h"
#include "AccountInfo.h"


class GameSession : public Session
{
public:
	~GameSession()
	{
		cout << "~GameSession" << endl;
	}

public:
	virtual void		on_disconnect() override;
	virtual void		on_send(int32 len) override;
	virtual void		on_recv_packet(BYTE* buffer, int32 len) override;

public:
	void set_is_login(bool is_login) { _is_login = is_login; }
	void set_name(wstring name) { _name = name; }
private:
	GameSessionRef get_game_session() { return static_pointer_cast<GameSession>(shared_from_this()); }

public:
	AccountInfo _account_info;
	wstring _name;
	bool _is_login = false;
};

