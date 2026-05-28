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
private:
	USE_LOCK;
	DBConnectRef db_connect;

private:
	SQLHENV henv;
	SQLHDBC hdbc;

};

