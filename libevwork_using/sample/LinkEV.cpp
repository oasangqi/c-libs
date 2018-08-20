#include "LinkEV.h"
#include "Types.h"
#include "Client.h"
#include "Log.h"
#include <algorithm>

extern struct g_args_s g_args;

void LinkEV::onConnected(evwork::IConn* pConn)
{
	if (g_args.conMap.find(pConn->getfd()) != g_args.conMap.end()) {
		Client *pClient = g_args.cmap[pConn->getfd()];
		DBUG(ERROR, "%s 与机器人:%d fd重复:%d", __FUNCTION__, pClient->m_uid, pConn->getfd());
	}

	g_args.conMap[pConn->getfd()] = pConn;

	Client *pClient = g_args.cmap[pConn->getfd()];
	DBUG(INFO, "机器人:%d fd:%d pConn:%p 连接成功\n", pClient->m_uid, pConn->getfd(), pConn);
	return;
}

void LinkEV::onClose(evwork::IConn* pConn)
{
	g_args.conMap.erase(pConn->getfd());

	for (std::map<int, struct gameSvrQueue_s>::iterator cit = g_args.queues.begin(); cit != g_args.queues.end(); cit++) {
		std::vector<int> &killQueue = cit->second.dels;
		std::vector<int>::iterator it = std::find(killQueue.begin(), killQueue.end(), pConn->getfd());
		if (it != killQueue.end()) {
			killQueue.erase(it);
			break;
		}
	}

	Client *pClient = g_args.cmap[pConn->getfd()];
	DBUG(INFO, "机器人:%d fd:%d pConn:%p 断开连接\n", pClient->m_uid, pConn->getfd(), pConn);
	delete pClient;
	return;
}
