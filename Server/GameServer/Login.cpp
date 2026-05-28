#include "pch.h"
#include "Login.h"

BYTE Login::create_account(string id, string pw, OUT uint64& account_id)
{
    WRITE_LOCK;

    if (!_memory_login_info_db.empty())
    {
        if (_memory_login_info_db.count(id)) {
            return Protocol::LOGIN_ERROR::LOGIN_CREATE_HAS_ACCOUNT;
        }
    }

    uint64 idx = _account_id.fetch_add(1);
    AccountInfo account_info(idx, id, pw);
    _memory_login_info_db.insert(make_pair(id, account_info));

    account_id = idx;

    return Protocol::LOGIN_ERROR::LOGIN_SUCCESS;
}

BYTE Login::login(string id, string pw, OUT uint64& account_id)
{
    WRITE_LOCK;

    if (_memory_login_info_db.empty()){
        return Protocol::LOGIN_ERROR::LOGIN_ID_NONE;
    }

    if (!_memory_login_info_db.count(id)) {
        return Protocol::LOGIN_ERROR::LOGIN_ID_NONE;
    }

    AccountInfo account_info = _memory_login_info_db.find(id)->second;
    if (account_info._pw != pw) {
        return Protocol::LOGIN_ERROR::LOGIN_PW_NONE;
    }

    account_id = _account_id.fetch_add(1);

    return  Protocol::LOGIN_ERROR::LOGIN_SUCCESS;
}
