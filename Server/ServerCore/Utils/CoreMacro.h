#pragma once

#define OUT

#define NAMESPACE_BEGIN(name)	namespace name {
#define NAMESPACE_END			}

/*---------------
	  Lock
---------------*/

#define USE_MANY_LOCKS(count)	mutex _locks[count];
#define USE_LOCK				USE_MANY_LOCKS(1)
#define	WRITE_LOCK_IDX(idx)		lock_guard<mutex> lockGuard_##idx(_locks[idx]);
#define WRITE_LOCK				WRITE_LOCK_IDX(0)

/*---------------
	  Crash
---------------*/

#define CRASH(cause)						\
{											\
	uint32* crash = nullptr;				\
	__analysis_assume(crash != nullptr);	\
	*crash = 0xDEADBEEF;					\
}

#define ASSERT_CRASH(expr)			\
{									\
	if (!(expr))					\
	{								\
		CRASH("ASSERT_CRASH");		\
		__analysis_assume(expr);	\
	}								\
}

/*---------------
	  Send
---------------*/

#define SEND_PACKET(pkt)												\
	SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(pkt); \
	session->send(sendBuffer);


/*---------------
	  Log
---------------*/
#define DEBUG_LOG(message) GLogger->write(LogLevel::LOG_DEBUG, message)
#define INFO_LOG(message)  GLogger->write(LogLevel::LOG_INFO, message)
#define WARN_LOG(message)  GLogger->write(LogLevel::LOG_WARN, message)
#define ERROR_LOG(message) GLogger->write(LogLevel::LOG_ERROR, message)	