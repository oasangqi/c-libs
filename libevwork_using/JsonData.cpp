#include "JsonData.h"

#include "EVWork.h"

using namespace evwork;

#define DEFAULT_PACKET_LIMIT 1024*1024*16
#define DEF_PRINT_INTERVAL	10

CJsonData::CJsonData()
: m_uProc(0)
, m_uPacketLimit(DEFAULT_PACKET_LIMIT)
, m_uBytes64(0)
{
	__initTimerPrint();
}
CJsonData::~CJsonData()
{
	__destroyTimerPrint();
}

void CJsonData::setPacketLimit(uint32_t uLimit)
{
	m_uPacketLimit = uLimit; 
}

int CJsonData::onData(IConn* pConn, const char* pData, size_t uSize)
{
	std::string strPeerIp = "";
	uint16_t uPeerPort = 0;
	pConn->getPeerInfo(strPeerIp, uPeerPort);

	int nProcessed = 0;

	while (uSize > 0)
	{
		// 接收的数据没包头长，返回0
		if (uSize < HEADER_SIZE)
			break;

		// 取报文长度
		// 协议格式: 协议头 + 协议数据，其中协议头用4字节表明协议数据的长度
		uint32_t uPktLen = Header::peekLen(pData);

		if ( m_uPacketLimit != (uint32_t)-1 && uPktLen > (m_uPacketLimit - HEADER_SIZE) )
		{
			LOG(Warn, "[CJsonData::%s] from:%s:%u packet pktlen:%u > limit:%u", __FUNCTION__, strPeerIp.c_str(), uPeerPort, uPktLen + HEADER_SIZE, m_uPacketLimit);

			return -1;
		}

		// 接收的数据没达到包头指明的长度，返回0
		if (uSize < HEADER_SIZE + uPktLen)
			break;

		{
			std::string strPacket(pData+HEADER_SIZE, uPktLen);

			Jpacket packet;
			if (packet.parse(strPacket) < 0)
			{
				LOG(Warn, "[CJsonData::%s] from:%s:%u recv a invalid packet, not json format", __FUNCTION__, strPeerIp.c_str(), uPeerPort);

				return -1;
			}

			// 匹配协议号进行处理
			__requestDispatch(packet, pConn);
		}

		// 有可能客户端一次发送的了多条协议的数据，还需要继续处理下条协议
		pData += (uPktLen + HEADER_SIZE);
		uSize -= (uPktLen + HEADER_SIZE);

		nProcessed += (int)(uPktLen + HEADER_SIZE);

		// 用于打印单位时间内CJsonData对象进行了多少次处理、处理了多少字节数据
		// CJsonData构造函数中调用__initTimerPrint()设置定时器
		m_uProc++;
		m_uBytes64 += (uPktLen + HEADER_SIZE);
	}

	return nProcessed;
}

void CJsonData::__requestDispatch(Jpacket& packet, IConn* pConn)
{
	if (getAppContext())
	{
		getAppContext()->RequestDispatch(packet, pConn);
	}
}

void CJsonData::__initTimerPrint()
{
	m_evTimerPrint.data = this;
	ev_timer_init(&m_evTimerPrint, CJsonData::__cbTimerPrint, DEF_PRINT_INTERVAL, DEF_PRINT_INTERVAL);
	ev_timer_start(CEnv::getEVLoop()->getEvLoop(), &m_evTimerPrint);
}

void CJsonData::__destroyTimerPrint()
{
	ev_timer_stop(CEnv::getEVLoop()->getEvLoop(), &m_evTimerPrint);
}

void CJsonData::__cbTimerPrint(struct ev_loop *loop, struct ev_timer *w, int revents)
{
	CJsonData* pThis = (CJsonData*)w->data;

	LOG(Info, "[CJsonData::%s] proc:%u bytes:%llu", __FUNCTION__, pThis->m_uProc, pThis->m_uBytes64);

	// 重置统计值
	pThis->m_uProc = 0;
	pThis->m_uBytes64 = 0;
}
