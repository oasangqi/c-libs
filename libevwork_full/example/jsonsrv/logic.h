#pragma once 

#include "libevwork/EVWork.h"
#include "libevwork/jsmfc/FormDef.h"
#include "libevwork/jsmfc/Sender.h"

#include "libevwork/TimerHandler.h"

class CLogic
	: public js::PHClass
	, public evwork::ILinkEvent
{
public:
	DECLARE_JS_FORM_MAP;

	CLogic();
	virtual ~CLogic();

	// 连接关注
	virtual void onConnected(evwork::IConn* pConn);
	virtual void onClose(evwork::IConn* pConn);

	// 协议处理
	void onMessage1(Json::Value* pJson, evwork::IConn* pConn);

private:

	// 模拟客户端定时发包
	virtual bool __onHandlerSender();

	// 定时器
	TimerHandler<CLogic, &CLogic::__onHandlerSender> m_timerSender;
};
