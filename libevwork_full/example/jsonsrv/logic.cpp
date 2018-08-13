#include "logic.h"

using namespace evwork;
using namespace js;

// 消息命令字定义
#define MESSAGE_ID_1	1
#define MESSAGE_ID_2	2

BEGIN_JS_FORM_MAP(CLogic)
	ON_JS_REQUEST_CONN(MESSAGE_ID_1, &CLogic::onMessage1)
END_JS_FORM_MAP()

CLogic::CLogic()
{
	// 初使化定时器
	m_timerSender.init(this);
	m_timerSender.start(10000); // 10S发一个包
}

CLogic::~CLogic()
{
	// 停止定时器
	m_timerSender.stop();
}

// 连接关注
void CLogic::onConnected(evwork::IConn* pConn)
{
	LOG(Info, "[CLogic::%s] #conn# conn:[%d]", __FUNCTION__, pConn->getcid());

	// 在这里可以做一个容器将连接记下来
}

void CLogic::onClose(evwork::IConn* pConn)
{
	LOG(Info, "[CLogic::%s] #conn# conn:[%d]", __FUNCTION__, pConn->getcid());

	// 在这里需要将连接从容器中擦除
}

// 协议处理
void CLogic::onMessage1(Json::Value* pJson, evwork::IConn* pConn)
{
	LOG(Info, "[CLogic::%s] conn:[%d] call...", __FUNCTION__, pConn->getcid());

	// 这里可以分析Json格式，取出数据做相应处理

	// 这里简单打印Json内容
	std::string strJsonText;
	{
		Json::FastWriter writer;
		strJsonText = writer.write(*pJson);

		LOG(Debug, "[CLogic::%s] message1:[%s]", __FUNCTION__, strJsonText.c_str());
	}

	// 发回响应，这里简单将原包发回
	{
		js::Sender sdr(MESSAGE_ID_2, strJsonText);
		pConn->sendBin(sdr.Data(), sdr.Size());
	}
}

// 模拟客户端定时发包
bool CLogic::__onHandlerSender()
{
	LOG(Info, "[CLogic::%s] call...", __FUNCTION__);

	std::string strJsonText = "{\"uid\":10000,\"shapass\":\"xxxxxxxxxxxxxxxxxxxx\"}";
	js::Sender sdr(MESSAGE_ID_1, strJsonText);

	std::string strTargetIp = "127.0.0.1";
	uint16_t uTargetPort = 1982;
	CEnv::getThreadEnv()->getWriter()->send(strTargetIp, uTargetPort, sdr.Data(), sdr.Size());

	// 返回值true表示定时器持续，否则停止
	return true;
}
