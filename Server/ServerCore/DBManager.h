#pragma once
#include <sqlext.h>
#include <sqltypes.h>
#include <sql.h>
#include <chrono>
#include "DBConnect.h"
#include "DBEnum.h"

class DBManager
{
public:
	DBManager();
	~DBManager();

public:
	bool DBConnectStart();
	bool SendPacket(SQLHSTMT& hStmt, SQLRETURN& retcode);

public:
	bool Insert_Accession(wstring id, wstring password);
	bool Select_LoginData(OUT uint64& idx, OUT wstring& id, OUT wstring& password);
	bool Insert_Player(OUT CREATE_PLAYER_ERROR& error, uint64 idx, wstring name, uint32 lv, float hp, float atk, float df);
	bool Select_Player(OUT SELECT_PLAYER_ERROR& error, OUT wstring& name, OUT uint32& lv, OUT float& hp, OUT float& atk, OUT float& df, uint64 idx);
private:
	USE_LOCK;
	DBConnectRef dbConnect;

private:
	SQLHENV hEnv;
	SQLHDBC hDbc;

};

