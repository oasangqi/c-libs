#include "RedisClient.h"

using namespace tinyredis;

CRedisClient::CRedisClient(const std::string& strIp, uint16_t uPort16, const std::string& strPass, uint32_t uMiniSeconds)
: m_strIp(strIp)
, m_uPort16(uPort16)
, m_strPass(strPass)
, m_uMiniSeconds(uMiniSeconds)
, m_pRedisClient(NULL)
{
}

CRedisClient::CRedisClient(const std::string& strPath, const std::string& strPass, uint32_t uMiniSeconds)
: m_strPath(strPath)
, m_strPass(strPass)
, m_uMiniSeconds(uMiniSeconds)
, m_pRedisClient(NULL)
{
}

CRedisClient::~CRedisClient()
{
	__tryDisconnect();
}

const std::string& CRedisClient::getErrStr()
{
	return m_strErrStr;
}

redisReply* CRedisClient::command(const char* szFormat, ...)
{
	int32_t nRetryCount = 2;

	do
	{
		if (--nRetryCount <= 0)
			break;

		if ((m_strIp.size() > 0 && !__tryConnect())
				|| (m_strPath.size() > 0 && !__tryConnectIPC()))
			continue;

		va_list ap;
		va_start(ap, szFormat);

		CResult result(true);
		result = (redisReply*)redisvCommand(m_pRedisClient, szFormat, ap);

		va_end(ap);

		int32_t nRet = __commPrepareReply(result.get());

		if (nRet == -1)
		{
			if (m_strIp.size() > 0)
				__makeErrorString("redis[%s:%hu] net error, desc:%s", m_strIp.c_str(), m_uPort16, m_pRedisClient->errstr);
			else if (m_strPath.size() > 0)
				__makeErrorString("redis[%s] IPC error, desc:%s", m_strPath.c_str(), m_pRedisClient->errstr);
			continue;
		}
		else if (nRet == 1)
		{
			if (m_strIp.size() > 0)
				__makeErrorString("redis[%s:%hu] command error, desc:%s", m_strIp.c_str(), m_uPort16, m_pRedisClient->errstr);
			else if (m_strPath.size() > 0)
				__makeErrorString("redis[%s] command error, desc:%s", m_strPath.c_str(), m_pRedisClient->errstr);
			break;
		}

		__makeErrorString("");
		return result.release();

	} while (0);

	return NULL;
}

bool CRedisClient::__tryConnect()
{
	if (m_pRedisClient)
		return true;

	struct timeval tvConnect;
	tvConnect.tv_sec = (m_uMiniSeconds/1000);
	tvConnect.tv_usec = (m_uMiniSeconds%1000)*1000;

	m_pRedisClient = redisConnectWithTimeout(m_strIp.c_str(), m_uPort16, tvConnect);

	if (m_pRedisClient == NULL || m_pRedisClient->err != REDIS_OK)
	{
		if (m_pRedisClient)
			__makeErrorString("redis[%s:%hu] connect failed, desc:%s", m_strIp.c_str(), m_uPort16, m_pRedisClient->errstr);
		else
			__makeErrorString("redis[%s:%hu] connect failed", m_strIp.c_str(), m_uPort16);

		__tryDisconnect();
		return false;
	}

	if (m_strPass != "" && !__auth())
	{
		__tryDisconnect();
		return false;
	}

	__makeErrorString("");
	return true;
}

bool CRedisClient::__tryConnectIPC()
{
	if (m_pRedisClient)
		return true;

	struct timeval tvConnect;
	tvConnect.tv_sec = (m_uMiniSeconds/1000);
	tvConnect.tv_usec = (m_uMiniSeconds%1000)*1000;

	m_pRedisClient = redisConnectUnixWithTimeout(m_strPath.c_str(), tvConnect);

	if (m_pRedisClient == NULL || m_pRedisClient->err != REDIS_OK)
	{
		if (m_pRedisClient)
			__makeErrorString("redis[%s] connect failed, desc:%s", m_strPath.c_str(), m_pRedisClient->errstr);
		else
			__makeErrorString("redis[%s] connect failed", m_strPath.c_str());

		__tryDisconnect();
		return false;
	}

	if (m_strPass != "" && !__auth())
	{
		__tryDisconnect();
		return false;
	}

	__makeErrorString("");
	return true;
}

void CRedisClient::__tryDisconnect()
{
	if (m_pRedisClient)
	{
		redisFree(m_pRedisClient);
		m_pRedisClient = NULL;
	}
}

bool CRedisClient::__auth()
{
	CResult result(true);
	result = (redisReply*)redisCommand(m_pRedisClient, "auth %s", m_strPass.c_str());

	if (!result)
		return false;

	if (!result.isStatus())
	{
		if (m_strIp.size() > 0)
			__makeErrorString("redis[%s:%hu] auth ****** failed, not support", m_strIp.c_str(), m_uPort16);
		else if (m_strPath.size() > 0)
			__makeErrorString("redis[%s] auth ****** failed, not support", m_strPath.c_str());

		return false;
	}

	if (!result.isOK())
	{
		std::string strStatus;
		result.getString(strStatus);

		if (m_strIp.size() > 0)
			__makeErrorString("redis[%s:%hu] auth ****** failed, response:%s", m_strIp.c_str(), m_uPort16, strStatus.c_str());
		else if (m_strPath.size() > 0)
			__makeErrorString("redis[%s] auth ****** failed, response:%s", m_strPath.c_str(), strStatus.c_str());

		return false;
	}

	__makeErrorString("");
	return true;
}

int32_t CRedisClient::__commPrepareReply(redisReply* pReply)
{
	if (pReply == NULL || m_pRedisClient->err != REDIS_OK)
	{
		__tryDisconnect();
		return -1; 
	}

	if (pReply->type == REDIS_REPLY_ERROR)
		return 1; 

	return 0;
}

void CRedisClient::__makeErrorString(const char* szFormat, ...)
{
	char szErrTmp[1024] = {0};

	va_list ap;
	va_start(ap, szFormat);
	vsnprintf(szErrTmp, 1024, szFormat, ap);
	va_end(ap);

	m_strErrStr = szErrTmp;
}


CResult::CResult(bool bAutoFree)
: m_pReply(NULL)
, m_bAutoFree(bAutoFree)
{
}
CResult::~CResult()
{
	if (m_bAutoFree)
		free();
}

void CResult::free()
{
	if (m_pReply)
	{
		freeReplyObject(m_pReply);
		m_pReply = NULL;
	}
}

redisReply* CResult::get()
{
	return m_pReply;
}

redisReply* CResult::release()
{
	redisReply* pRet = m_pReply;
	m_pReply = NULL;

	return pRet;
}

bool CResult::isArray()
{
	return (m_pReply->type == REDIS_REPLY_ARRAY);
}

bool CResult::isInteger()
{
	return (m_pReply->type == REDIS_REPLY_INTEGER);
}

bool CResult::isString()
{
	return (m_pReply->type == REDIS_REPLY_STRING);
}

bool CResult::isNil()
{
	return (m_pReply->type == REDIS_REPLY_NIL);
}

bool CResult::isStatus()
{
	return (m_pReply->type == REDIS_REPLY_STATUS);
}

redisReply* CResult::getSubReply(size_t uPos)
{
	return m_pReply->element[uPos];
}

size_t CResult::getArraySize()
{
	return m_pReply->elements;
}

int64_t CResult::getInteger()
{
	return m_pReply->integer;
}

void CResult::getString(std::string& str)
{
	str.assign(m_pReply->str, m_pReply->len);
}

bool CResult::isOK()
{
	std::string str;
	getString(str);
	return (str == "OK");
}

bool CResult::operator ! ()
{
	return (m_pReply == NULL);
}

void CResult::operator = (redisReply *pReply)
{
	free();
	
	m_pReply = pReply;
}
