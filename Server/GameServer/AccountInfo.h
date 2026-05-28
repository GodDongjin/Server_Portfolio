#pragma once
#include "pch.h"

struct AccountInfo
{
	AccountInfo() = default;

	AccountInfo(uint64 account_id, string id, string pw) { _account_id = account_id; _id = id; _pw = pw; };

	uint64 _account_id = 0;
	string _id;
	string _pw;
};