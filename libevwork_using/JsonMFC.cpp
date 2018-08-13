#include "JsonMFC.h"

#include "EVWork.h"

#include <assert.h>

using namespace evwork;

CJsonMFC::CJsonMFC()
{
}
CJsonMFC::~CJsonMFC()
{
}

void CJsonMFC::addEntry(FormEntry* pEntry, void* pTarget)
{
	for (int i = 0; pEntry[i].m_uCmd != 0; ++i)
	{
		if (m_mapEntry.find(pEntry[i].m_uCmd) != m_mapEntry.end())
		{
			LOG(Error, "[CJsonMFC::%s] dupicate form entry, cmd:%u", __FUNCTION__, pEntry[i].m_uCmd);
			assert(false);
		}

		pEntry[i].m_pObj = (PHClass*)pTarget;
		m_mapEntry[pEntry[i].m_uCmd] = &pEntry[i];
	}
}

void CJsonMFC::RequestDispatch(Jpacket& packet, IConn* pConn)
{
	uint32_t uCmd = -1;
	try
	{
		if (packet.val.isObject())
		{
			uCmd = packet.val.get("cmd", -1).asInt();
		}
	}
	catch (...)
	{
	}
	
	if (uCmd == (uint32_t)-1)
	{
		LOG(Error, "[CJsonMFC::%s] json format error, no cmd field", __FUNCTION__);
		return;
	}

	// 寻找协议号对应的协议处理函数
	ENTRY_MAP_t::iterator iter = m_mapEntry.find(uCmd);
	if (iter == m_mapEntry.end())
	{
		DefaultDispatch(packet, pConn);
		return;
	}

	FormEntry* pEntry = iter->second;

	TargetProc prc;
	prc.mf_oo = pEntry->m_pFunc; // TOSTUDY

	switch (pEntry->m_uType)
	{
	case fpt_vv:
		(pEntry->m_pObj->*prc.mf_vv)();
		break;
	case fpt_vc:
		(pEntry->m_pObj->*prc.mf_vc)(packet);
		break;
	case fpt_vcc:
		// 调用对应协议处理函数(类的函数)
		(pEntry->m_pObj->*prc.mf_vcc)(packet, pConn);
		break;
	default:
		LOG(Warn, "[CMfcAppContext::%s] request entry: %d not compare fpt type", __FUNCTION__, uCmd);
	}
}

void CJsonMFC::DefaultDispatch(Jpacket& packet, IConn* pConn)
{
	LOG(Warn, "[CJsonMFC::%s] not find request entry: %d", __FUNCTION__, packet.val["cmd"].asInt());
}
