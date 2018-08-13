#include "libevwork/EVWork.h"

#include "libevwork/jsmfc/DataHandler.h"
#include "libevwork/jsmfc/MfcAppContext.h"

#include "logic.h"

using namespace evwork;

int main(int argc, char* argv[])
{
	//-------------------------------------------------------------------------
	// libevwork初使化

	signal(SIGPIPE, SIG_IGN);

	CSyslogReport LG;
	CEVLoop LP;
	CConnManager CM;
	CWriter WR;

	CEnv::getThreadEnv()->setLogger(&LG);
	CEnv::getThreadEnv()->setEVLoop(&LP);
	CEnv::getThreadEnv()->setLinkEvent(&CM);
	CEnv::getThreadEnv()->setConnManager(&CM);
	CEnv::getThreadEnv()->setWriter(&WR);

	LP.init();

	//-------------------------------------------------------------------------
	// 应用程序初使化

	// 设置连接超时，单位S
	CEnv::getThreadEnv()->getEVParam().uConnTimeout = 300; 

	// 定义数据包处理器
	js::CDataHandler __DE;

	// 数据包处理器限制请求包的最大长度，单位字节
	__DE.setPacketLimit(16*1024); 

	// 关联数据包处理器
	CEnv::getThreadEnv()->setDataEvent(&__DE);

	// 设置MFC对象
	js::CMfcAppContext __MFC;

	// 将MFC对象关联到数据包处理器
	__DE.setAppContext(&__MFC);

	// 定义逻辑主对象
	CLogic __logic;

	// 逻辑主对象关注连接事件
	// 可用于处理应用层关联连接和玩家对象
	CM.addLE(&__logic);

	// 将主对象的消息映射表装载到MFC对象
	// 同样的，可以定义别的逻辑对象，也装载到MFC对象中
	__MFC.addEntry(CLogic::getFormEntries(), &__logic);

	// 创建服务套接口对象
	CListenConn __listenConn(1982); 

	//-------------------------------------------------------------------------
	// 启动事件循环

	LP.runLoop();

	return 0;
}
