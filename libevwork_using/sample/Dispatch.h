#ifndef _DISPATCH_H_
#define _DISPATCH_H_

#include "../FormDef.h"

class CDispatch
	: public evwork::PHClass
{
public:
	DECLARE_FORM_MAP;

	// 通用协议处理
	void onEnterRoomReply(evwork::Jpacket& packet, evwork::IConn* pConn);
	void onEnterRoomFail(evwork::Jpacket& packet, evwork::IConn* pConn);
	void onReadyReply(evwork::Jpacket& packet, evwork::IConn* pConn);
	void onReadyFail(evwork::Jpacket& packet, evwork::IConn* pConn);
	void onStartRound(evwork::Jpacket& packet, evwork::IConn* pConn);
	void onTableInfoReply(evwork::Jpacket& packet, evwork::IConn* pConn);
	void onNextRound(evwork::Jpacket& packet, evwork::IConn* pConn);
	void onChangeTableFail(evwork::Jpacket& packet, evwork::IConn* pConn);
	void onLogoutOK(evwork::Jpacket& packet, evwork::IConn* pConn);

	// 非通用协议处理
	void onInitHandCards(evwork::Jpacket& packet, evwork::IConn* pConn);
	void onBankerLastCard(evwork::Jpacket& packet, evwork::IConn* pConn);
	void onNoticeOutCard(evwork::Jpacket& packet, evwork::IConn* pConn);
	void onSomeOneOutCard(evwork::Jpacket& packet, evwork::IConn* pConn);
	void onDoAction(evwork::Jpacket& packet, evwork::IConn* pConn);
	void onSomeOneEat(evwork::Jpacket& packet, evwork::IConn* pConn);
	void onOutCardOK(evwork::Jpacket& packet, evwork::IConn* pConn);
	void onSyncHandCards(evwork::Jpacket& packet, evwork::IConn* pConn);
};

#endif
