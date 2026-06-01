#pragma once
#include "pch.h"
#include "AccountInfo.h"
#include "Protocol.pb.h"

class Login
{
public:
	BYTE create_account(string id, string pw, wstring user_name, OUT uint64& account_id);
	BYTE login(string id, string pw, OUT wstring& user_name, OUT uint64& account_id);

	bool logout(uint64 account_id);
private:
	USE_LOCK;
	unordered_map<string, AccountInfo> _memory_login_info_db;
	atomic<uint64> _account_id{ 1 };

	set<uint64> _login_list;
};

