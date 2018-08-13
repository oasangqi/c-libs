﻿#include "EVComm.h"
#include "EVWork.h"

using namespace evwork;

CEVLoop::CEVLoop()
: m_pEVLoop(NULL)
{
}
CEVLoop::~CEVLoop()
{
	destroy();
}

bool CEVLoop::init()
{
	if (m_pEVLoop == NULL)
	{
		m_pEVLoop = ev_default_loop(0);
	}

	return (m_pEVLoop != NULL);
}

void CEVLoop::destroy()
{
	if (m_pEVLoop)
		m_pEVLoop = NULL;
}

void CEVLoop::runLoop()
{
	if (m_pEVLoop)
		ev_loop(m_pEVLoop, 0);
}

void CEVLoop::setHandle(IHandle* p)
{
	if (m_setHandle.find(p) != m_setHandle.end())
		return;

	ev_io_init(&p->getEvIo(), IHandle::evCallBack, p->getFd(), p->getEv());
	ev_io_start(m_pEVLoop, &p->getEvIo());

	m_setHandle.insert(p);
}

void CEVLoop::delHandle(IHandle* p)
{
	if (m_setHandle.find(p) == m_setHandle.end())
		return;

	ev_io_stop(m_pEVLoop, &p->getEvIo());

	m_setHandle.erase(p);
}

struct ev_loop* CEVLoop::getEvLoop()
{
	return m_pEVLoop;
}
