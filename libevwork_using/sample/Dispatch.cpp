#include <algorithm>
#include "Dispatch.h"
#include "protocol.h"
#include "Client.h"
#include "../EVWork.h"

using namespace evwork;
extern struct g_args_s g_args;

BEGIN_FORM_MAP(CDispatch)
	ON_REQUEST_CONN(SERVER_LOGIN_BC, &CDispatch::onEnterRoomReply)
	ON_REQUEST_CONN(SERVER_LOGIN_FAIL, &CDispatch::onEnterRoomFail)
	ON_REQUEST_CONN(SERVER_READY_BC, &CDispatch::onReadyReply)
	ON_REQUEST_CONN(SERVER_READY_FAIL, &CDispatch::onReadyFail)
	ON_REQUEST_CONN(SERVER_GAME_START_BC, &CDispatch::onStartRound)
	ON_REQUEST_CONN(SERVER_TABLE_INFO_UC, &CDispatch::onTableInfoReply)
	ON_REQUEST_CONN(SERVER_GAME_ENDGAME_BC, &CDispatch::onNextRound)
	ON_REQUEST_CONN(SERVER_CHANGE_TABLE_FAIL, &CDispatch::onChangeTableFail)
	ON_REQUEST_CONN(SERVER_LOGOUT_OK_BC, &CDispatch::onLogoutOK)

	ON_REQUEST_CONN(SERVER_GAME_PLAYCARD_BC, &CDispatch::onInitHandCards)
	ON_REQUEST_CONN(SERVER_GAME_BANKERCARD_BC, &CDispatch::onBankerLastCard)
	ON_REQUEST_CONN(SERVER_GAME_OUTCARD_BC, &CDispatch::onNoticeOutCard)
	ON_REQUEST_CONN(SERVER_GAME_SOMEONE_OUT_CARD_BC, &CDispatch::onSomeOneOutCard)
	ON_REQUEST_CONN(SERVER_GAME_SOMEONE_EAT_BC, &CDispatch::onSomeOneEat)
	ON_REQUEST_CONN(SERVER_GAME_ACTLIST_UC, &CDispatch::onDoAction)
	ON_REQUEST_CONN(SERVER_GAME_OUTCARD_OK_BC, &CDispatch::onOutCardOK)
	ON_REQUEST_CONN(SERVER_GAME_SYNC_HANDCARDS_UC, &CDispatch::onSyncHandCards)
END_FORM_MAP()

void CDispatch::onEnterRoomReply(evwork::Jpacket& packet, evwork::IConn* pConn)
{
	Client	*pclient = findClientByfd(pConn->getfd());
	pclient->cbEnterRoom(packet);
}

void CDispatch::onEnterRoomFail(evwork::Jpacket& packet, evwork::IConn* pConn)
{
	Client	*pclient = findClientByfd(pConn->getfd());
	pclient->cbEnterRoomFail(packet);
}

void CDispatch::onChangeTableFail(evwork::Jpacket& packet, evwork::IConn* pConn)
{
	Client	*pclient = findClientByfd(pConn->getfd());
	pclient->cbChangeTableFail(packet);
}

void CDispatch::onLogoutOK(evwork::Jpacket& packet, evwork::IConn* pConn)
{
	Client	*pclient = findClientByfd(pConn->getfd());
	pclient->cbLogoutOK(packet);
}

void CDispatch::onReadyReply(evwork::Jpacket& packet, evwork::IConn* pConn)
{
	Client	*pclient = findClientByfd(pConn->getfd());
	pclient->cbReady(packet);
}

void CDispatch::onReadyFail(evwork::Jpacket& packet, evwork::IConn* pConn)
{
	Client	*pclient = findClientByfd(pConn->getfd());
	pclient->cbReadyFail(packet);
}

void CDispatch::onStartRound(evwork::Jpacket& packet, evwork::IConn* pConn)
{
	Client	*pclient = findClientByfd(pConn->getfd());
	pclient->log(INFO, "开始游戏");
}

void CDispatch::onInitHandCards(evwork::Jpacket& packet, evwork::IConn* pConn)
{
	Client	*pclient = findClientByfd(pConn->getfd());
	pclient->cbInitHandCards(packet);
}

void CDispatch::onBankerLastCard(evwork::Jpacket& packet, evwork::IConn* pConn)
{
	Client	*pclient = findClientByfd(pConn->getfd());
	Json::Value &val = packet.tojson();

	int banker = val["banker_seatid"].asInt();
	int card = val["card"].asInt();

	pclient->log(INFO, "庄家座位[%d], 挡底牌[%d]", banker, card);

}

void CDispatch::onTableInfoReply(evwork::Jpacket& packet, evwork::IConn* pConn)
{
	Client	*pclient = findClientByfd(pConn->getfd());
	pclient->cbTableInfo(packet);

}

void CDispatch::onNoticeOutCard(evwork::Jpacket& packet, evwork::IConn* pConn)
{
	Client	*pclient = findClientByfd(pConn->getfd());
	pclient->cbNoticeOutCard(packet);
}

void CDispatch::onSomeOneOutCard(evwork::Jpacket& packet, evwork::IConn* pConn)
{
	Client	*pclient = findClientByfd(pConn->getfd());
	pclient->cbSomeOneOutCard(packet);
}

// TODO:目前只碰、胡
void CDispatch::onDoAction(evwork::Jpacket& packet, evwork::IConn* pConn)
{
	Client	*pclient = findClientByfd(pConn->getfd());
	pclient->cbAction(packet);
}

void CDispatch::onSomeOneEat(evwork::Jpacket& packet, evwork::IConn* pConn)
{
	Client	*pclient = findClientByfd(pConn->getfd());
	pclient->cbSomeOneEat(packet);
}

void CDispatch::onNextRound(evwork::Jpacket& packet, evwork::IConn* pConn)
{
	Client	*pclient = findClientByfd(pConn->getfd());
	pclient->cbNextRound(packet);
}

void CDispatch::onOutCardOK(evwork::Jpacket& packet, evwork::IConn* pConn)
{
	Client	*pclient = findClientByfd(pConn->getfd());
	pclient->cbOutCardOK(packet);
}

void CDispatch::onSyncHandCards(evwork::Jpacket& packet, evwork::IConn* pConn)
{
	Client	*pclient = findClientByfd(pConn->getfd());
	pclient->cbSyncHandCards(packet);
}
