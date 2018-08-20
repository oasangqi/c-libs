#include "ConnManager.h"
#include "EVWork.h"

using namespace evwork;

CConnManager::CConnManager()
: m_uLastConnId(1)
{
}

CConnManager::~CConnManager()
{
}

// 通过cid获取已建立的连接
IConn* CConnManager::getConnById(uint32_t uConnId)
{
	std::map<uint32_t, IConn*>::iterator iter = m_mapCIdConn.find(uConnId);
	if (iter == m_mapCIdConn.end())
		return NULL;

	return iter->second;
}

// 建立连接
IConn* CConnManager::connectServer(const std::string& strIp, uint16_t uPort)
{
	CClientConn* pConn = new CClientConn(strIp, uPort);

	if (pConn->getfd() < 0) {
		delete pConn;
		return NULL;
	}

	return pConn;
}

void CConnManager::onConnected(IConn* pConn)
{
	// 设置连接ID
	pConn->setcid(m_uLastConnId);

	// 保存新连接的指针
	m_mapCIdConn[m_uLastConnId] = pConn;

	m_uLastConnId++;

	// 连接后行为
	__notifyLEConnected(pConn);
}

void CConnManager::onClose(IConn* pConn)
{
	// 断开前行为
	__notifyLEClose(pConn);

	m_mapCIdConn.erase( pConn->getcid() );
}

void CConnManager::addLE(ILinkEvent* p)
{
	m_setLE.insert(p);
}

void CConnManager::delLE(ILinkEvent* p)
{
	m_setLE.erase(p);
}

void CConnManager::__notifyLEConnected(IConn* pConn)
{
	for (std::set<ILinkEvent*>::iterator iter = m_setLE.begin(); iter != m_setLE.end(); ++iter)
	{
		(*iter)->onConnected(pConn);
	}
}

void CConnManager::__notifyLEClose(IConn* pConn)
{
	for (std::set<ILinkEvent*>::iterator iter = m_setLE.begin(); iter != m_setLE.end(); ++iter)
	{
		(*iter)->onClose(pConn);
	}
}

