#include "pch.h"
#include <sqlext.h>
#include <sqltypes.h>
#include <sql.h>

class DBConnect : public enable_shared_from_this<DBConnect>
{
public:
	DBConnect();
	~DBConnect();


	bool connect(SQLHENV& henv, SQLHDBC& hdbc);

private:
	SQLRETURN _retcode;
};
