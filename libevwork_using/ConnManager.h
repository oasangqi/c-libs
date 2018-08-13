#pragma once 

#include "EVComm.h"

#include <map>

namespace evwork
{

	class CConnManager
		: public IConnManager
		, public ILinkEvent
	{
	public:
		CConnManager();
		virtual ~CConnManager();

		virtual IConn* getConnById(uint32_t uConnId);
		virtual IConn* getConnByIpPort(const std::string& strIp, uint16_t uPort);
		virtual IConn* getConnByIpPort2(const std::string& strIp, uint16_t uPort);

		// 调用业务层注册的所有连接前后的行为
		virtual void onConnected(IConn* pConn);
		virtual void onClose(IConn* pConn);

		// 注册业务层在连接前后的行为，可以注册多个行为
		// 注册方式: 实现ILinkEvent的子类，实现虚函数onConnected、onConnected，并调用该接口
		void addLE(ILinkEvent* p);
		void delLE(ILinkEvent* p);

	private:

		void __notifyLEConnected(IConn* pConn);
		void __notifyLEClose(IConn* pConn);

		std::string __toIpPortKey(const std::string& strIp, uint16_t uPort);

	protected:
		uint32_t m_uLastConnId;	// 分配每个TCP连接对应的m_cid，即通过连接getcid()的返回值

		std::map<uint32_t, IConn*> m_mapCIdConn;
		std::map<std::string, IConn*> m_mapIpPortConn;

		std::set<ILinkEvent*> m_setLE;
	};

}
