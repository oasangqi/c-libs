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
	IConn* pConn = CEnv::getConnManager()->connectServer(strIp, uPort);
	pConn->sendBin(pData, uSize);
	// 此时未必连接成功，getcid不可靠
	if (CEnv::getConnManager()->getConnById(pConn->getcid())) {
		// 连接肯定成功了
	}
	return pConn->getfd();
}
