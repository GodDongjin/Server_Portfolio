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
	bool db_connect_start();
	bool send_packet(SQLHSTMT& hStmt, SQLRETURN& _retcode);

public:
	bool insert_accession(wstring id, wstring password);
	bool select_login_data(OUT uint64& idx, OUT wstring& id, OUT wstring& password);
	bool insert_player(OUT CREATE_PLAYER_ERROR& error, uint64 idx, wstring name, uint32 lv, float hp, float atk, float df);
	bool select_player(OUT SELECT_PLAYER_ERROR& error, OUT wstring& name, OUT uint32& lv, OUT float& hp, OUT float& atk, OUT float& df, uint64 idx);
private:
	USE_LOCK;
	DBConnectRef db_connect;

private:
	SQLHENV henv;
	SQLHDBC hdbc;

};

