#ifndef _CLIENT_H_
#define _CLIENT_H_

#include <stdint.h>
#include <json/json.h>
#include "libev/ev.h"
#include "Types.h"
#include "Cardlib/handCards.h"
#include "../EVWork.h"
#include "../Logger.h"
#include "../JsonPacket.h"
#include "libtinyredis/RedisFactory.h"

using namespace evwork;

class Client 
{
	public:
		Client(int uid);
		~Client();

		void readyLater();
		void cancelReady();
		void cbAction(Jpacket& packet);
		void cbEnterRoom(Jpacket& packet);
		void cbEnterRoomFail(Jpacket& packet);
		void cbChangeTableFail(Jpacket& packet);
		void cbReady(Jpacket& packet);
		void cbReadyFail(Jpacket& packet);
		void cbInitHandCards(Jpacket& packet);
		void cbTableInfo(Jpacket& packet);
		void cbNextRound(Jpacket& packet);
		void cbLogoutOK(Jpacket& packet);

		void cbSomeOneEat(Jpacket& packet);
		void cbNoticeOutCard(Jpacket& packet);
		void cbSomeOneOutCard(Jpacket& packet);
		void cbOutCardOK(Jpacket& packet);
		void cbSyncHandCards(Jpacket& packet);

		void	log(int level, const char *fmt, ...);
		short	m_fd;
		int m_uid;
		uint8_t		m_seatid;
		HandCards handcards;

		friend class CDispatch;


	private:
		bool	m_playing;
		int	m_logfd;
		evwork::IConn	*m_pConn;
		char	m_msgbuf[MSG_BUFF_SIZE];
		char    m_logbuf[LOG_BUFF_SIZE];

		void __doLogin();
		void __doHeartBeat();
		void __beReady();

		void __doAction();
		void __outCard();
		bool __wiseEat(Json::Value& val, int& chiIdx, int& biIdx);
		void __doLogout();
		void __disConnectLater();
		void __logoutLater(int low, int max);

		void __dumpHandCards();

		static void __cbTimerLogin(struct ev_loop *loop, struct ev_timer *w, int revents);
		static void __cbTimerLogout(struct ev_loop *loop, struct ev_timer *w, int revents);
		static void __cbTimerHeartBeat(struct ev_loop *loop, struct ev_timer *w, int revents);
		static void __cbTimerReady(struct ev_loop *loop, struct ev_timer *w, int revents);
		static void __cbTimerAction(struct ev_loop *loop, struct ev_timer *w, int revents);
		static void __cbTimerOutCard(struct ev_loop *loop, struct ev_timer *w, int revents);

		ev_timer m_evTimerLogin;
		ev_timer m_evTimerLogout;
		ev_timer m_evTimerHeartBeat;
		ev_timer m_evTimerReady;

		Jpacket packetAction;
		ev_timer m_evTimerAction;

		ev_timer m_evTimerOutCard;

		void	open_log();
		void	close_log();
		void	_startTimers();
		void	_stopTimers();
};

Client *findClientByfd(int fd);
#endif
