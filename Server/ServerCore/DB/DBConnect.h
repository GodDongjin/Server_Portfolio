#include <sqlext.h>
#include <sqltypes.h>
#include <sql.h>

class DBConnect : public enable_shared_from_this<DBConnect>
{
public:
	DBConnect();
	~DBConnect();


	bool connect(SQLHENV& henv, SQLHDBC& hdbc, const wstring driver, const wstring ip, 
		const wstring database, const wstring user, const wstring pw);

private:
	SQLRETURN _retcode;
};
