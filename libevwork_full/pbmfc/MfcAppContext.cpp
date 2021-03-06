﻿//============================================================================
// Name        : MfcAppContext.cpp
// Author      : kdjie
// Version     : 1.0
// Copyright   : @2015
// Description : 14166097@qq.com
//============================================================================

#include "MfcAppContext.h"

#include "../EVWork.h"

using namespace evwork;
using namespace pb;

CMfcAppContext::CMfcAppContext()
{
}

CMfcAppContext::~CMfcAppContext()
{
}

void CMfcAppContext::addEntry(FormEntry* pEntry, void* pTarget)
{
	for (int i = 0; pEntry[i].m_uCmd != 0; ++i)
	{
		if (m_mapEntry.find(pEntry[i].m_uCmd) != m_mapEntry.end())
		{
			LOG(Error, "[pb::CMfcAppContext::%s] dupicate form entry, cmd:%u", __FUNCTION__, pEntry[i].m_uCmd);
			assert(false);
		}

		pEntry[i].m_pObj = (PHClass*)pTarget;
		m_mapEntry[pEntry[i].m_uCmd] = &pEntry[i];
	}
}

void CMfcAppContext::RequestDispatch(Request& request, evwork::IConn* pConn)
{
	ENTRY_MAP_t::iterator iter = m_mapEntry.find(request.getCmd());
	if (iter == m_mapEntry.end())
	{
		DefaultDispatch(request, pConn);
		return;
	}

	FormEntry* pEntry = iter->second;

	void* pPacket = pEntry->m_pFormHandle.get()->handlePacket(request.Body(), request.BodySize());

	TargetProc prc;
	prc.mf_oo = pEntry->m_pFunc;

	switch (pEntry->m_uType)
	{
	case fpt_vv:
		(pEntry->m_pObj->*prc.mf_vv)();
		break;
	case fpt_vc:
		(pEntry->m_pObj->*prc.mf_vc)(pPacket);
		break;
	case fpt_vcc:
		(pEntry->m_pObj->*prc.mf_vcc)(pPacket, pConn);
		break;
	default:
		LOG(Warn, "[pb::CMfcAppContext::%s] request entry: %u not compare fpt type", __FUNCTION__, request.getCmd());
	}

	pEntry->m_pFormHandle.get()->destroyFrom(pPacket);
}

void CMfcAppContext::DefaultDispatch(Request& request, evwork::IConn* pConn)
{
	LOG(Warn, "[pb::CMfcAppContext::%s] not find request entry: %d", __FUNCTION__, request.getCmd());
}
