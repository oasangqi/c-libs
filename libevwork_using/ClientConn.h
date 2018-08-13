#pragma once 

#include "EVComm.h"

namespace evwork
{

	class CClientConn
		: public IConn
	{
	public:
		CClientConn(const std::string& strPeerIp, uint16_t uPeerPort16);	// 作为客户端连接服务器
		CClientConn(int fd, const std::string& strPeerIp, uint16_t uPeerPort16);	// 作为服务器处理客户端连接
		virtual ~CClientConn();
		
		virtual void getPeerInfo(std::string& strPeerIp, uint16_t& uPeerPort16);
		virtual bool sendBin(const char* pData, size_t uSize);

		void cbEvent(int revents);

	private:

		void __noblock();

		void __initTimerNoData();
		void __destroyTimerNoData();
		void __updateTimerNoData();
		static void __cbTimerNoData(struct ev_loop *loop, struct ev_timer *w, int revents);

		void __initTimerDestry();
		void __destroyTimerDestry();
		static void __cbTimerDestry(struct ev_loop *loop, struct ev_timer *w, int revents);

		void __onError();

		void __onRead();

		void __onWrite();

		void __appendBuffer(const char* pData, size_t uSize);

		void __sendBuffer();

		size_t __sendData(const char* pData, size_t uSize);

		size_t __recvData(char* pData, size_t uSize);

		void __willFreeMyself(const std::string& strDesc);

	private:
		std::string m_strPeerIp;
		uint16_t m_uPeerPort16;

		bool m_bConnected; // 是否已连接，作为客户端时初始化为false，作为服务器时初始化为true

		std::string m_strInput;
		std::string m_strOutput;

		ev_timer m_evTimerNoData;	// 超时定时器，超时后断开连接
		ev_timer m_evTimerDestroy;
		bool m_bTimerNoDataStart;	// m_evTimerNoData定时器是否已设置
		bool m_bTimerDestroyStart;

		THandle<CClientConn, &CClientConn::cbEvent> m_hRead; // 连接读事件对应回调cbEvent
		THandle<CClientConn, &CClientConn::cbEvent> m_hWrite;// 连接写事件也对应回调cbEvent
		bool m_wevRegisted;
	};

}
