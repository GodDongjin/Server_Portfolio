#include "pch.h"
#include "Login.h"

BYTE Login::create_account(string id, string pw, wstring user_name, OUT uint64& account_id)
{
    WRITE_LOCK;

    if (!_memory_login_info_db.empty())
    {
        if (_memory_login_info_db.count(id)) {
            return Protocol::LOGIN_ERROR::LOGIN_CREATE_HAS_ACCOUNT;
        }
    }

    uint64 idx = _account_id.fetch_add(1);
    AccountInfo account_info(idx, id, pw, user_name);
    _memory_login_info_db.insert(make_pair(id, account_info));

    account_id = idx;
    _login_list.insert(account_id);

    return Protocol::LOGIN_ERROR::LOGIN_SUCCESS;
}

BYTE Login::login(string id, string pw, OUT wstring& user_name, OUT uint64& account_id)
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

    if (_login_list.count(account_info._account_id))
        return Protocol::LOGIN_ERROR::LOGIN_ALREADY_LOGIN;

    account_id = account_info._account_id;
    user_name = account_info._user_name;
    _login_list.insert(account_id);

    return  Protocol::LOGIN_ERROR::LOGIN_SUCCESS;
}

bool Login::logout(uint64 account_id)
{
    WRITE_LOCK;

    if (_login_list.empty()) {
        return false;
    }

    if (!_login_list.count(account_id)) {
        return false;
    }

    _login_list.erase(account_id);

    return true;
}
