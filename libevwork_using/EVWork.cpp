#include "EVWork.h"

using namespace evwork;

CEVLoop* CEnv::m_spEVLoop = NULL;
ILogReport* CEnv::m_spLogger = NULL;
ILinkEvent* CEnv::m_spLinkEvent = NULL;
IDataEvent* CEnv::m_spDataEvent = NULL;
IWriter* CEnv::m_spWriter = NULL;
IConnManager* CEnv::m_spConnManager = NULL;
SEVParam CEnv::m_sEVParam;

void CEnv::setEVLoop(CEVLoop* p)
{
	m_spEVLoop = p;
}

CEVLoop* CEnv::getEVLoop()
{
	return m_spEVLoop;
}

void CEnv::setLogger(ILogReport* p)
{
	m_spLogger = p;
}

ILogReport* CEnv::getLogger()
{
	return m_spLogger;
}

void CEnv::setLinkEvent(ILinkEvent* p)
{
	m_spLinkEvent = p;
}

ILinkEvent* CEnv::getLinkEvent()
{
	return m_spLinkEvent;
}

void CEnv::setDataEvent(IDataEvent* p)
{
	m_spDataEvent = p;
}

IDataEvent* CEnv::getDataEvent()
{
	return m_spDataEvent;
}

void CEnv::setWriter(IWriter* p)
{
	m_spWriter = p;
}

IWriter* CEnv::getWriter()
{
	return m_spWriter;
}

void CEnv::setConnManager(IConnManager* p)
{
	m_spConnManager = p;
}

IConnManager* CEnv::getConnManager()
{
	return m_spConnManager;
}

SEVParam& CEnv::getEVParam()
{
	return m_sEVParam;
}
