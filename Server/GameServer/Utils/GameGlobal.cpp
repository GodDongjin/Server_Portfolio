#include "pch.h"
#include "GameGlobal.h"
#include "../Login/Login.h"

Login* GLogin = nullptr;

class GameGlobal
{
public:
	GameGlobal()
	{
		GLogin = new Login();
	}

	~GameGlobal()
	{
		delete GLogin;
	}
}GGameGlobal;