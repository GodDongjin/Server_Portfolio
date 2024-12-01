#include "pch.h"
#include <sqlext.h>
#include <sqltypes.h>
#include <sql.h>

class DBConnect : public enable_shared_from_this<DBConnect>
{
public:
	DBConnect();
	~DBConnect();


	bool Connect(SQLHENV& hEnv, SQLHDBC& hDbc);

private:
	SQLRETURN retcode;
};
