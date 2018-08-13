#include "Writer.h"

#include "EVWork.h"

using namespace evwork;

CWriter::CWriter()
{
}
CWriter::~CWriter()
{
}

int CWriter::send(const std::string& strIp, uint16_t uPort, const char* pData, size_t uSize)
{
	IConn* pConn = CEnv::getConnManager()->getConnByIpPort2(strIp, uPort);
	pConn->sendBin(pData, uSize);
	return pConn->getfd();
}
