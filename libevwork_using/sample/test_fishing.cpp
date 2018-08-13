#include "EVWork.h"

#include "../JsonPacket.h"

using namespace evwork;

class CDataEvent
	: public IDataEvent
{
public:
	CDataEvent(int _uid, const std::string& _strIp, uint16_t _uPort)
	{
		m_uid = _uid;
		m_strIp = _strIp;
		m_uPort16 = _uPort;

		m_evTimerLogin.data = this;
		ev_timer_init(&m_evTimerLogin, CDataEvent::__cbTimerLogin, 1, 60);
		ev_timer_start(CEnv::getEVLoop()->getEvLoop(), &m_evTimerLogin);

		m_evTimerShot.data = this;
		ev_timer_init(&m_evTimerShot, CDataEvent::__cbTimerShot, 0.1, 0.1);
		ev_timer_start(CEnv::getEVLoop()->getEvLoop(), &m_evTimerShot);

		bLogined = false;
		m_seatid = -1;
		m_bid = 1;
	}
	~CDataEvent()
	{
		ev_timer_stop(CEnv::getEVLoop()->getEvLoop(), &m_evTimerLogin);

		ev_timer_stop(CEnv::getEVLoop()->getEvLoop(), &m_evTimerShot);
	}

	virtual int onData(IConn* pConn, const char* pData, size_t uSize)
	{
		std::string strPeerIp = "";
		uint16_t uPeerPort = 0;
		pConn->getPeerInfo(strPeerIp, uPeerPort);

		int nProcessed = 0;

		while (uSize > 0)
		{
			if (uSize < HEADER_SIZE)
				break;

			uint32_t uPktLen = Header::peekLen(pData);

			if (uSize < uPktLen)
				break;

			{
				std::string strPacket(pData+HEADER_SIZE, uPktLen);

				Jpacket packet;
				if (packet.parse(strPacket) < 0)
				{
					LOG(Warn, "[CDataEvent::%s] from:%s:%u recv a invalid packet, not json format", __FUNCTION__, strPeerIp.c_str(), uPeerPort);

					return -1;
				}

				Json::FastWriter writer;
				std::string strJsonText = writer.write(packet.val);
				LOG(Info, "[CDataEvent::%s] from:%s:%u recv data:[%s]", __FUNCTION__, strPeerIp.c_str(), uPeerPort, strJsonText.c_str());

				int cmd = packet.val["cmd"].asInt();
				if (cmd == 4003)
				{
					for (int i = 0; i < (int)packet.val["players"].size(); ++i)
					{
						Json::Value& valPlayer = packet.val["players"][i];
						
						if (valPlayer["uid"].asInt() == m_uid)
						{
							m_seatid = valPlayer["seatid"].asInt();
							bLogined = true;

							LOG(Info, "==== uid:%d seatid:%d", m_uid, m_seatid);
						}
					}
				}
			}

			pData += (uPktLen + HEADER_SIZE);
			uSize -= (uPktLen + HEADER_SIZE);

			nProcessed += (int)(uPktLen + HEADER_SIZE);
		}

		return nProcessed;
	}

	void login()
	{
		bLogined = false;

		{
			Jpacket packet;
			packet.val["cmd"] = 1002;
			packet.end();
			CEnv::getWriter()->send(m_strIp, m_uPort16, packet.tostring().data(), packet.tostring().size());
		}

		{
			Jpacket packet;
			packet.val["cmd"] = 1001;
			packet.val["uid"] = m_uid;
			packet.end();

			CEnv::getWriter()->send(m_strIp, m_uPort16, packet.tostring().data(), packet.tostring().size());
		}

		{
			Jpacket packet;
			packet.val["cmd"] = 1003;
			packet.end();

			CEnv::getWriter()->send(m_strIp, m_uPort16, packet.tostring().data(), packet.tostring().size());
		}
	}

	void shot()
	{
		if (!bLogined)
			return;

		srand((unsigned)time(NULL));

		{
			Jpacket packet;
			packet.val["cmd"] = 1005;

			int x = 320, y = 50, d = 45;

			if (m_seatid == 1)
			{
				x = 384;
				y = 51;
				d = 45;
			}
			else if (m_seatid == 2)
			{
				x = 896;
				y = 51;
				d = 45;
			}
			else if (m_seatid == 3)
			{
				x = 896;
				y = 750;
				d = 215;
			}
			else if (m_seatid == 4)
			{
				x = 384;
				y = 750;
				d = 215;
			}

			int du = d + rand() % 90;

			packet.val["touchx"] = -1;
			packet.val["touchy"] = -1;

			for (int i = 0; i < 3; ++i)
			{
				packet.val["bullets"][i]["id"] = m_bid++;
				packet.val["bullets"][i]["rate"] = 10;
				packet.val["bullets"][i]["d"] = du;
				packet.val["bullets"][i]["x"] = x + 10*i;
				packet.val["bullets"][i]["y"] = y;
			}

			packet.end();
			CEnv::getWriter()->send(m_strIp, m_uPort16, packet.tostring().data(), packet.tostring().size());
		}

		{
			Jpacket packet;
			packet.val["cmd"] = 1006;
			for (int i = 0; i < 3; ++i)
			{
				packet.val["bullets"][i]["id"] = m_bid - 50 + i;
				packet.val["bullets"][i]["fids"].append( Json::Value(1) );
			}

			packet.end();
			CEnv::getWriter()->send(m_strIp, m_uPort16, packet.tostring().data(), packet.tostring().size());
		}
	}

	static void __cbTimerLogin(struct ev_loop *loop, struct ev_timer *w, int revents)
	{
		CDataEvent* pThis = (CDataEvent*)w->data;
		pThis->login();
	}

	static void __cbTimerShot(struct ev_loop *loop, struct ev_timer *w, int revents)
	{
		CDataEvent* pThis = (CDataEvent*)w->data;
		pThis->shot();
	}

private:
	ev_timer m_evTimerLogin;
	ev_timer m_evTimerShot;
	bool bLogined;

	int m_uid;
	std::string m_strIp;
	uint16_t m_uPort16;

	int m_seatid;
	int m_bid;
};

int main(int argc, char* argv[])
{
	//-------------------------------------------------------------------------
	// libevwork初使化

	CSyslogReport LG;
	CEVLoop LP;
	CConnManager CM;
	CWriter WR;

	CEnv::setLogger(&LG);
	CEnv::setEVLoop(&LP);
	CEnv::setLinkEvent(&CM);
	CEnv::setConnManager(&CM);
	CEnv::setWriter(&WR);

	LP.init();

	//-------------------------------------------------------------------------
	// 应用程序初使化

	CEnv::getEVParam().uConnTimeout = 300;

	int uid = atoi(argv[1]);
	std::string strIp = argv[2];
	uint16_t uPort16 = atoi(argv[3]);

	CDataEvent DE(uid, strIp, uPort16);
	CEnv::setDataEvent(&DE);

	//CListenConn listenConn(1981);

	//-------------------------------------------------------------------------
	// 启动事件循环

	LP.runLoop();

	return 0;
}
