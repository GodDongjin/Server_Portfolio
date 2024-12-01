#include "pch.h"
#include "DBManager.h"

DBManager::DBManager()
{
	dbConnect = make_shared<DBConnect>();
}

DBManager::~DBManager()
{
}

bool DBManager::DBConnectStart()
{
	if (hDbc != nullptr || hEnv != nullptr)
		return false;

	return dbConnect->Connect(hEnv, hDbc);;
}


bool DBManager::SendPacket(SQLHSTMT& hStmt, SQLRETURN& retcode)
{
    retcode = SQLExecute(hStmt);
    if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
        std::cout << "Packet sent successfully." << std::endl;
    }
    else {
        std::cerr << "Failed to send packet." << std::endl;
        return false;
    }

    return true;
}

std::wstring SQLCharToWString(SQLCHAR* sqlCharStr, size_t length) {
    // SQLCHAR 배열을 string으로 변환
    std::string str(reinterpret_cast<char*>(sqlCharStr), length);

    // UTF-8에서 UTF-16(wstring)으로 변환
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    return converter.from_bytes(str);
}

bool DBManager::Insert_Accession(wstring id, wstring password)
{
    WRITE_LOCK;
    SQLHSTMT hStmt;
    SQLRETURN retcode;

    SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);

    // 쿼리 실행
    retcode = SQLPrepareW(hStmt, (SQLWCHAR*)L"CALL p_Login_Insert(?, ?)", SQL_NTS);
    wstring loginId = id;
    wstring loginPass = password;

    SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR, loginId.size(), 0, (SQLWCHAR*)loginId.c_str(), 0, NULL);
    SQLBindParameter(hStmt, 2, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR, loginPass.size(), 0, (SQLWCHAR*)loginPass.c_str(), 0, NULL);

    if (!SendPacket(hStmt, retcode)) {
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        return false;
    }

    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
    return true;
}

bool DBManager::Select_LoginData(OUT uint64& idx, OUT wstring& id, OUT wstring& password)
{
    WRITE_LOCK;

    SQLHSTMT hStmt;
    SQLRETURN retcode;

    SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);

    // 쿼리 실행
    retcode = SQLPrepareW(hStmt, (SQLWCHAR*)L"CALL p_Login_Select(?, ?)", SQL_NTS);
    wstring loginId = id;
    wstring loginPass = password;

    SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR, loginId.size(), 0, (SQLWCHAR*)loginId.c_str(), 0, NULL);
    SQLBindParameter(hStmt, 2, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR, loginPass.size(), 0, (SQLWCHAR*)loginPass.c_str(), 0, NULL);

    if (!SendPacket(hStmt, retcode)) {
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        return false;
    }

    SQLLEN outIdxLen = 0;
    SQLLEN outIdLen = 0;
    SQLLEN outPassLen = 0;
    while (SQLFetch(hStmt) == SQL_SUCCESS) {
        SQLGetData(hStmt, 1, SQL_C_UBIGINT, &idx, sizeof(idx), &outIdxLen);
        SQLGetData(hStmt, 2, SQL_C_WCHAR, (SQLWCHAR*)loginId.c_str(), sizeof(outIdLen), &outIdLen);
        SQLGetData(hStmt, 3, SQL_C_WCHAR, (SQLWCHAR*)loginPass.c_str(), sizeof(outPassLen), &outPassLen);
    } 

    if (outIdLen == 0 || outPassLen == 0)
    {
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        return false;
    }

    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);

    return true;
}

bool DBManager::Select_Player(OUT SELECT_PLAYER_ERROR& error, OUT wstring& name, OUT uint32& lv, OUT float& hp, OUT float& atk, OUT float& df, uint64 idx)
{
    WRITE_LOCK;
    SQLHSTMT hStmt;
    SQLRETURN retcode;

    SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);

    // 쿼리 실행 - p_Player_Select 호출
    retcode = SQLPrepareW(hStmt, (SQLWCHAR*)L"CALL p_Player_Select(?)", SQL_NTS);
    SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT, SQL_C_UBIGINT, SQL_BIGINT, 0, 0, &idx, 0, NULL);

    if (!SendPacket(hStmt, retcode)) {
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        error = SELECT_PLAYER_ERROR::SELECT_PLAYER_FAIL;
        return false;
    }

    // 결과 컬럼 선언 및 길이 변수
    SQLCHAR outName[13];
    SQLINTEGER outLv, outHp, outAtk, outDf;
    SQLLEN outNameLen, outLvLen, outHpLen, outAtkLen, outDfLen;

    // 결과 값 가져오기
    bool fetchSuccess = false;
    while (SQLFetch(hStmt) == SQL_SUCCESS) {
        fetchSuccess = true;
        SQLGetData(hStmt, 1, SQL_C_CHAR, outName, sizeof(outName), &outNameLen);
        SQLGetData(hStmt, 2, SQL_C_SLONG, &outLv, 0, &outLvLen);
        SQLGetData(hStmt, 3, SQL_C_SLONG, &outHp, 0, &outHpLen);
        SQLGetData(hStmt, 4, SQL_C_SLONG, &outAtk, 0, &outAtkLen);
        SQLGetData(hStmt, 5, SQL_C_SLONG, &outDf, 0, &outDfLen);
    }

    // 데이터가 비어 있는지 확인
    if (!fetchSuccess || outNameLen == SQL_NULL_DATA || outLvLen == SQL_NULL_DATA ||
        outHpLen == SQL_NULL_DATA || outAtkLen == SQL_NULL_DATA || outDfLen == SQL_NULL_DATA) {
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        error = SELECT_PLAYER_ERROR::SELECT_PLAYER_FAIL;
        return false;
    }

    // 가져온 데이터 할당
    name = SQLCharToWString(outName, outNameLen);
    lv = static_cast<uint32>(outLv);
    hp = static_cast<float>(outHp);
    atk = static_cast<float>(outAtk);
    df = static_cast<float>(outDf);

    error = SELECT_PLAYER_ERROR::SELECT_PLAYER_SUCCESS;
    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
    return true;
}

bool DBManager::Insert_Player(OUT CREATE_PLAYER_ERROR& error, uint64 idx, wstring name, uint32 lv, float hp, float atk, float df)
{
    WRITE_LOCK;
    SQLHSTMT hStmt;
    SQLRETURN retcode;

    SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);

    // 쿼리 실행
    retcode = SQLPrepareW(hStmt, (SQLWCHAR*)L"CALL p_Player_Insert(?, ?, ?, ?, ?, ?)", SQL_NTS);
    wstring playerName = name;

    SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT, SQL_C_UBIGINT, SQL_BIGINT, 0, 0, &idx, 0, NULL);
    SQLBindParameter(hStmt, 2, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR, name.size(), 0, (SQLWCHAR*)name.c_str(), 0, NULL);
    SQLBindParameter(hStmt, 3, SQL_PARAM_INPUT, SQL_C_ULONG, SQL_INTEGER, 0, 0, &lv, 0, NULL);
    SQLBindParameter(hStmt, 4, SQL_PARAM_INPUT, SQL_C_FLOAT, SQL_FLOAT, 0, 0, &hp, 0, NULL);
    SQLBindParameter(hStmt, 5, SQL_PARAM_INPUT, SQL_C_FLOAT, SQL_FLOAT, 0, 0, &atk, 0, NULL);
    SQLBindParameter(hStmt, 6, SQL_PARAM_INPUT, SQL_C_FLOAT, SQL_FLOAT, 0, 0, &df, 0, NULL);

    if (!SendPacket(hStmt, retcode)) {
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        error = CREATE_PLAYER_ERROR::CREATE_PLAYER_FAIL;
        return false;
    }

    SQLINTEGER result;
    SQLLEN resultLen = 0;
    // 6. 결과 받기 (ROW_COUNT()에 따른 결과)
    if (SQLFetch(hStmt) == SQL_SUCCESS) {
        SQLGetData(hStmt, 1, SQL_C_LONG, &result, 0, &resultLen);  // 불리언 값을 받음

        if (result == 1) {
            error = CREATE_PLAYER_ERROR::CREATE_PLAYER_SUCCESS;
        }
        else if (result == 0) {
            error = CREATE_PLAYER_ERROR::CREATE_PLAYER_DUPLICATION;
            return false;
        }
    }

    return true;
}

