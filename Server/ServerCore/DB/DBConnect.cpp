#include "pch.h"
#include "DBConnect.h"

DBConnect::DBConnect()
{
}

DBConnect::~DBConnect()
{
}

bool DBConnect::connect(SQLHENV& henv, SQLHDBC& hdbc, const wstring driver, const wstring ip, const wstring database, const wstring user, const wstring pw)
{
    // ODBC 환경 초기화
    SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);
    SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0);

    // 데이터베이스 연결을 위한 핸들 할당
    SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);

    // 연결 문자열 생성
    const std::wstring connectionString =
        L"DRIVER={" + driver + L"};"
        L"SERVER=" + ip + L";"
        L"DATABASE=" + database + L";"
        L"USER=" + user + L";"
        L"PASSWORD=" + pw + L";";

    // 연결 설정
    _retcode = SQLDriverConnectW(hdbc, NULL, 
        reinterpret_cast<SQLWCHAR*>(const_cast<wchar_t*>(connectionString.c_str())),
        SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT);

    if (_retcode == SQL_SUCCESS || _retcode == SQL_SUCCESS_WITH_INFO) {
        std::wcout << L"Connected to MySQL successfully!" << std::endl;
        // 연결 성공 후 필요한 작업 수행
    }
    else {
        std::cerr << "Failed to connect to MySQL." << std::endl;
        return false;
    }

    // 데이터베이스 연결 해제
    //SQLDisconnect(hdbc);

    // 핸들 해제
    //SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
    //SQLFreeHandle(SQL_HANDLE_ENV, henv);

    return true;

}

