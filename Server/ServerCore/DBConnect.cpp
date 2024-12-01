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
    // ODBC ȯ�� �ʱ�ȭ
    SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &hEnv);
    SQLSetEnvAttr(hEnv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0);

    // �����ͺ��̽� ������ ���� �ڵ� �Ҵ�
    SQLAllocHandle(SQL_HANDLE_DBC, hEnv, &hDbc);

    // ���� ���ڿ� ����
    wchar_t connectionString[] = L"DRIVER={MySQL ODBC 9.0 Unicode Driver};SERVER=localhost;DATABASE=2drpg;USER=root;PASSWORD=Qew0135@;";

    // ���� ����
    retcode = SQLDriverConnectW(hDbc, NULL, connectionString, SQL_NTS,
        NULL, 0, NULL, SQL_DRIVER_NOPROMPT);

    if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
        std::wcout << L"Connected to MySQL successfully!" << std::endl;
        // ���� ���� �� �ʿ��� �۾� ����
    }
    else {
        std::cerr << "Failed to connect to MySQL." << std::endl;
        return false;
    }

    // �����ͺ��̽� ���� ����
    //SQLDisconnect(hDbc);

    // �ڵ� ����
    //SQLFreeHandle(SQL_HANDLE_DBC, hDbc);
    //SQLFreeHandle(SQL_HANDLE_ENV, hEnv);

    return true;

}

