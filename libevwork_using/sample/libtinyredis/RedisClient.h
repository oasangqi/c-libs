#pragma once

#include <stdint.h>
#include <stdarg.h>

#include <iostream>
#include <sstream>
#include <string>

#include <hiredis/hiredis.h>

namespace tinyredis
{

	class CRedisClient
	{
	public:
		CRedisClient(const std::string& strIp, uint16_t uPort16, const std::string& strPass, uint32_t uMiniSeconds);
		virtual ~CRedisClient();

		const std::string& getErrStr();

		redisReply* command(const char* szFormat, ...);

	private:

		bool __tryConnect();
		void __tryDisconnect();
		bool __auth();

		int32_t __commPrepareReply(redisReply* pReply);

		void __makeErrorString(const char* szFormat, ...);

	private:
		std::string m_strIp;
		uint16_t m_uPort16;
		std::string m_strPass;
		uint32_t m_uMiniSeconds;

		redisContext* m_pRedisClient;

		std::string m_strErrStr;
	};

	class CResult
	{
	private:
		redisReply *m_pReply;
		bool m_bAutoFree;

	public:
		CResult(bool bAutoFree = true);
		virtual ~CResult();
		
		void free();
		redisReply* get();
		redisReply* release();

		bool isArray();
		bool isInteger();
		bool isString();
		bool isNil();
		bool isStatus();

		redisReply* getSubReply(size_t uPos);

		size_t getArraySize();
		int64_t getInteger();
		void getString(std::string& str);
		bool isOK();

		bool operator ! ();
		void operator = (redisReply *pReply);
	};

	template <typename R, typename T>
	static R convert(const T& t)
	{
		R r;

		std::stringstream ss;
		ss << t;
		ss >> r;

		return r;
	}

}
