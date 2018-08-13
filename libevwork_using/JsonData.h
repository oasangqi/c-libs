#pragma once 

#include "FormDef.h"

namespace evwork
{

	class CJsonData
		: public IDataEvent
		, public IAppContextAware
	{
	public:
		CJsonData();
		virtual ~CJsonData();

		void setPacketLimit(uint32_t uLimit);

		virtual int onData(IConn* pConn, const char* pData, size_t uSize);

	private:

		virtual void __requestDispatch(Jpacket& packet, IConn* pConn);

		void __initTimerPrint();
		void __destroyTimerPrint();
		static void __cbTimerPrint(struct ev_loop *loop, struct ev_timer *w, int revents);

	protected:
		uint32_t m_uProc;
		uint32_t m_uPacketLimit;	// 单条协议接收数据包最大大小，不含协议头(uint32_t)

		uint64_t m_uBytes64;

		ev_timer m_evTimerPrint;
	};

}
