#pragma once 

#include "EVComm.h"
#include "Logger.h"
#include "ListenConn.h"
#include "ClientConn.h"
#include "Writer.h"
#include "ConnManager.h"

namespace evwork
{
	struct SEVParam
	{
		uint32_t uConnTimeout;	// 客户端连接超时时间

		SEVParam()
		{
			uConnTimeout = (uint32_t)-1;
		}
	};

	class CEnv
	{
	public:
		static void setEVLoop(CEVLoop* p);
		static CEVLoop* getEVLoop();

		static void setLogger(ILogReport* p);
		static ILogReport* getLogger();

		static void setLinkEvent(ILinkEvent* p);
		static ILinkEvent* getLinkEvent();

		static void setDataEvent(IDataEvent* p);
		static IDataEvent* getDataEvent();

		static void setWriter(IWriter* p);
		static IWriter* getWriter();

		static void setConnManager(IConnManager* p);
		static IConnManager* getConnManager();

		static SEVParam& getEVParam();

	private:
		static CEVLoop* m_spEVLoop;
		static ILogReport* m_spLogger;
		static ILinkEvent* m_spLinkEvent;
		static IDataEvent* m_spDataEvent;
		static IWriter* m_spWriter;
		static IConnManager* m_spConnManager;
		static SEVParam m_sEVParam;
	};

	#define LOG(l,f,...) CEnv::getLogger()->log(l, f, __VA_ARGS__)

}
