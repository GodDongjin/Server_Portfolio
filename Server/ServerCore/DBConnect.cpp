#include "pch.h"
#include "DBConnect.h"

DBConnect::DBConnect()
{
}

DBConnect::~DBConnect()
{
}

bool DBConnect::Connect(SQLHENV& hEnv, SQLHDBC& hDbc)
{
    // ODBC 환경 초기화
    SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &hEnv);
    SQLSetEnvAttr(hEnv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0);

    // 데이터베이스 연결을 위한 핸들 할당
    SQLAllocHandle(SQL_HANDLE_DBC, hEnv, &hDbc);

    // 연결 문자열 생성
    wchar_t connectionString[] = L"DRIVER={MySQL ODBC 9.0 Unicode Driver};SERVER=localhost;DATABASE=2drpg;USER=root;PASSWORD=Qew0135@;";

    // 연결 설정
    retcode = SQLDriverConnectW(hDbc, NULL, connectionString, SQL_NTS,
        NULL, 0, NULL, SQL_DRIVER_NOPROMPT);

    if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
        std::wcout << L"Connected to MySQL successfully!" << std::endl;
        // 연결 성공 후 필요한 작업 수행
    }
    else {
        std::cerr << "Failed to connect to MySQL." << std::endl;
        return false;
    }

    // 데이터베이스 연결 해제
    //SQLDisconnect(hDbc);

    // 핸들 해제
    //SQLFreeHandle(SQL_HANDLE_DBC, hDbc);
    //SQLFreeHandle(SQL_HANDLE_ENV, hEnv);

    return true;

}

