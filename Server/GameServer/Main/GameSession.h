#pragma once
#include "../ServerCore/NetWork/Session.h"
#include "../Struct_info/AccountInfo.h"

class GameSession : public Session
{
public:
	~GameSession()
	{
	}

public:
	virtual void		on_disconnect() override;
	virtual void		on_send(int32 len) override;
	virtual void		on_recv_packet(BYTE* buffer, int32 len) override;
	virtual void		send_ping(uint64 now) override; // 클라이언트에 ping 전송

public:
	void set_is_login(bool is_login) { _is_login = is_login; }
	bool get_is_login() { return _is_login; }

private:
	GameSessionRef get_game_session() { return static_pointer_cast<GameSession>(shared_from_this()); }

private:
	AccountInfo _account_info;
	atomic<bool> _is_login = false;
};

