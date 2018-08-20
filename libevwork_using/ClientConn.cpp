#include "ClientConn.h"
#include <netdb.h>

#include "EVWork.h"

using namespace evwork;

#define DEF_CONN_TIMEOUT	60	// 默认客户端连接超时时间
#define MAX_INPUT_SIZE		8*1024*1024	// 一次最大接收字节数（客户端一次可能发送多条协议的数据），可以利用缓冲区读多次，每次追加到string对象
#define MAX_OUTPUT_SIZE		8*1024*1024	// 一次最大发送数据大小
#define READ_BUFF_SIZE		8*1024		// 读包缓冲区大小，读完后追加到string对象

CClientConn::CClientConn(const std::string& strPeerIp, uint16_t uPeerPort16)
: m_strPeerIp(strPeerIp)
, m_uPeerPort16(uPeerPort16)
, m_bConnected(false)
, m_strInput("")
, m_strOutput("")
, m_bTimerNoDataStart(false)
, m_bTimerDestroyStart(false)
, m_hRead(this) // 调用THandle类的构造函数
, m_hWrite(this)
, m_wevRegisted(false)
{
	m_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (m_fd == -1)
		throw exception_errno( toString("[CClientConn::%s] socket(ip:%s port:%u)", __FUNCTION__, m_strPeerIp.c_str(), m_uPeerPort16) );

	__noblock();

	struct sockaddr_in sinto;
	sinto.sin_family = AF_INET;
	sinto.sin_addr.s_addr = ::inet_addr(m_strPeerIp.c_str());
	sinto.sin_port = htons(m_uPeerPort16);

	if (!__connect()) {
		close(m_fd);
		m_fd = -1;
	}

	m_hRead.setEv(EV_READ);
	m_hRead.setFd(m_fd);

	m_hWrite.setEv(EV_WRITE);
	m_hWrite.setFd(m_fd);

	CEnv::getEVLoop()->setHandle(&m_hWrite);
	m_wevRegisted = true;
}

CClientConn::CClientConn(int fd, const std::string& strPeerIp, uint16_t uPeerPort16)
: m_strPeerIp(strPeerIp)
, m_uPeerPort16(uPeerPort16)
, m_bConnected(true)
, m_strInput("")
, m_strOutput("")
, m_bTimerNoDataStart(false)
, m_bTimerDestroyStart(false)
, m_hRead(this)
, m_hWrite(this)
, m_wevRegisted(false)
{
	m_fd = fd;

	__noblock();

	m_hRead.setEv(EV_READ);
	m_hRead.setFd(m_fd);

	m_hWrite.setEv(EV_WRITE);
	m_hWrite.setFd(m_fd);

	// 新的连接首先注册读事件
	CEnv::getEVLoop()->setHandle(&m_hRead);

	// 设置超时定时器
	__initTimerNoData();

	// 调用虚基类指针(ILinkEvent*)指向的onConnected函数，例如csmj中为CConnManager类的onConnected函数，
	// ConnManager类的onConnected再调用通过addLE注册的所有onConnected函数
	if (CEnv::getLinkEvent())
		CEnv::getLinkEvent()->onConnected(this);
}

CClientConn::~CClientConn()
{
	CEnv::getEVLoop()->delHandle(&m_hRead);
	if (m_wevRegisted) {
		CEnv::getEVLoop()->delHandle(&m_hWrite);
	}

	__destroyTimerNoData();
	__destroyTimerDestry();

	// 调用虚基类指针(ILinkEvent*)指向的onClose函数，例如csmj中为CConnManager类的onClose函数
	// 因为CClientConn为IConn的子类，所以客户端连接可通过delete (IConn *)断开
	if (CEnv::getLinkEvent())
		CEnv::getLinkEvent()->onClose(this);

	if (m_fd != -1)
	{
		close(m_fd);
		m_fd = -1;
	}
}

void CClientConn::getPeerInfo(std::string& strPeerIp, uint16_t& uPeerPort16)
{
	strPeerIp = m_strPeerIp;
	uPeerPort16 = m_uPeerPort16;
}

bool CClientConn::sendBin(const char* pData, size_t uSize)
{
	try
	{
		// 待发送数据追加到待发送string
		__appendBuffer(pData, uSize);

		if (m_bConnected)
		{
			try
			{
				// 发送数据
				__sendBuffer();
			}
			catch (exception_errno& e)
			{
				__willFreeMyself( toString("exception:%s", e.what()) );
				return false;
			}
		}

		return true;
	}
	catch (exception_errno& e)
	{
		LOG(Info, "[CClientConn::%s] catch exception:[%s]", __FUNCTION__, e.what());

		// 异常，延迟断开连接
		__initTimerDestry();

		return false;
	}
}

void CClientConn::cbEvent(int revents)
{
	// 处理EV_ERROR事件
	if (revents & EV_ERROR)
	{
		__onError();
		return;
	}
	
	if (revents & EV_READ)
	{
		__onRead();
	}

	if (revents & EV_WRITE)
	{
		__onWrite();
	}
}

void CClientConn::__noblock()
{
	int nFlags = fcntl(m_fd, F_GETFL);
	if (nFlags == -1)
		throw exception_errno( toString("[CListenConn::%s] fcntl(%d, F_GETFL) failed!", __FUNCTION__, m_fd) );

	nFlags |= O_NONBLOCK;

	int nRet = fcntl(m_fd, F_SETFL, nFlags);
	if (nRet == -1)
		throw exception_errno( toString("[CListenConn::%s] fcntl(%d, F_SETFL) failed!", __FUNCTION__, m_fd) );
}

// 非阻塞连接，即使返回true，也不一定连上了(可能服务器繁忙或是压根就连不上)
bool CClientConn::__connect()
{
	bool	ret = true;
	char _port[6];  /* strlen("65535"); */
	struct addrinfo hints, *servinfo, *p; 

	snprintf(_port, 6, "%d", m_uPeerPort16);
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	if (getaddrinfo(m_strPeerIp.c_str(), _port, &hints, &servinfo) != 0) {
		return false;
	} 
	for (p = servinfo; p != NULL; p = p->ai_next) {
		if (connect(m_fd, p->ai_addr, p->ai_addrlen) != 0
				&& errno != EINPROGRESS) {
			continue;
		}
		goto end;
	}
	if (p == NULL) {
		LOG(Info, "[CClientConn::%s] can't connect:[%s:%d]", __FUNCTION__, m_strPeerIp.c_str(), m_uPeerPort16);
	}
	ret = false;
end:
	freeaddrinfo(servinfo);
	return ret;
}

void CClientConn::__initTimerNoData()
{
	if (m_bTimerNoDataStart)
		return;

	float fTimeout = DEF_CONN_TIMEOUT;
	if (CEnv::getEVParam().uConnTimeout != (uint32_t)-1)
	{
		// 使用设置的超时时间
		fTimeout = CEnv::getEVParam().uConnTimeout;
	}

	m_evTimerNoData.data = this;
	ev_timer_init(&m_evTimerNoData, CClientConn::__cbTimerNoData, fTimeout, fTimeout);
	ev_timer_start(CEnv::getEVLoop()->getEvLoop(), &m_evTimerNoData);

	m_bTimerNoDataStart = true;
}

void CClientConn::__destroyTimerNoData()
{
	if (!m_bTimerNoDataStart)
		return;

	ev_timer_stop(CEnv::getEVLoop()->getEvLoop(), &m_evTimerNoData);

	m_bTimerNoDataStart = false;
}

void CClientConn::__updateTimerNoData()
{
	if (!m_bTimerNoDataStart)
		return;

	// 重置timer，效率高于stop/start
	ev_timer_again(CEnv::getEVLoop()->getEvLoop(), &m_evTimerNoData);
}

void CClientConn::__cbTimerNoData(struct ev_loop *loop, struct ev_timer *w, int revents)
{
	CClientConn* pThis = (CClientConn*)w->data;

	LOG(Info, "[CClientConn::%s] fd:[%d] peer:[%s:%u] timeout", __FUNCTION__, pThis->m_fd, pThis->m_strPeerIp.c_str(), pThis->m_uPeerPort16);

	pThis->__willFreeMyself( "timeout" );
}

void CClientConn::__initTimerDestry()
{
	if (m_bTimerDestroyStart)
		return;

	m_evTimerDestroy.data = this;
	ev_timer_init(&m_evTimerDestroy, CClientConn::__cbTimerDestry, 0.1, 0);
	ev_timer_start(CEnv::getEVLoop()->getEvLoop(), &m_evTimerDestroy);

	m_bTimerDestroyStart = true;
}

void CClientConn::__destroyTimerDestry()
{
	if (!m_bTimerDestroyStart)
		return;

	ev_timer_stop(CEnv::getEVLoop()->getEvLoop(), &m_evTimerDestroy);

	m_bTimerDestroyStart = false;
}

void CClientConn::__cbTimerDestry(struct ev_loop *loop, struct ev_timer *w, int revents)
{
	CClientConn* pThis = (CClientConn*)w->data;

	LOG(Info, "[CClientConn::%s] fd:[%d] peer:[%s:%u] destroy", __FUNCTION__, pThis->m_fd, pThis->m_strPeerIp.c_str(), pThis->m_uPeerPort16);

	pThis->__willFreeMyself( "destroy" );
}

void CClientConn::__onError()
{
	__willFreeMyself( "on Error" );
}

void CClientConn::__onRead()
{
	// 收到数据了，重置定时器
	__updateTimerNoData();

	// 循环一次最多读取READ_BUFF_SIZE字节数据，一次读事件最大读取MAX_INPUT_SIZE字节
	while (true)
	{
		char szBuf[READ_BUFF_SIZE] = {0};
		size_t uBytesRecv = 0;

		try
		{
			// 接收客户端数据
			uBytesRecv = __recvData(szBuf, READ_BUFF_SIZE);
		}
		// TOSTUDY
		catch (exception_errno& e)
		{
			__willFreeMyself( toString("exception:%s", e.what()) );
			return;
		}

		if (uBytesRecv > 0)
		{
			// 接收的数据过大，关闭连接
			if (m_strInput.size() + uBytesRecv > MAX_INPUT_SIZE)
			{
				__willFreeMyself( "input buffer flow!!!" );
				return;
			}

			// 追加(可能多次触发读事件)到度缓冲字符串
			m_strInput.append(szBuf, uBytesRecv);
		}

		if (uBytesRecv < READ_BUFF_SIZE)
			break;
		// 如果本轮循环读到了READ_BUFF_SIZE字节的数据，说明还可能有数据，还应该继续读
	}

	if (!m_strInput.empty())
	{
		int nRetSize = 0;

		if (CEnv::getDataEvent())
			// 处理接收的数据
			nRetSize = CEnv::getDataEvent()->onData(this, m_strInput.data(), m_strInput.size());
		else
			nRetSize = m_strInput.size();

		if (nRetSize < 0)
		{
			__willFreeMyself( "onData" );
		}
		else if (nRetSize > 0)
		{
			m_strInput.erase(0, nRetSize);
		}
		// nRetSize==0代表本条协议还需要继续接收数据，本次接收的数据不能清空，
		// 因为客户端可能分多次发送一条协议的数据
	}
}

void CClientConn::__onWrite()
{
	// 当作客户端使用时才会进入以下判断，开始发送数据了说明连接成功了
	if (!m_bConnected)
	{
		int e = 0;
		socklen_t l = sizeof(e);

		getsockopt(m_fd, SOL_SOCKET, SO_ERROR, &e, &l);
		if (e)
		{
			__willFreeMyself( "connect failed" );
			return;
		}

		m_bConnected = true;

		CEnv::getEVLoop()->setHandle(&m_hRead);

		__initTimerNoData();

		if (CEnv::getLinkEvent())
			CEnv::getLinkEvent()->onConnected(this);
	}

	try
	{
		// 发送数据
		__sendBuffer();
	}
	catch (exception_errno& e)
	{
		__willFreeMyself( toString("exception:%s", e.what()) );
		return;
	}

	
	// 发完数据成功后取消注册的写事件，否则下次还会触发(只要缓冲区不满)写事件，
	// 有之后触发的写回调将未发送完成的数据发完
	if (m_strOutput.empty())
	{
		CEnv::getEVLoop()->delHandle(&m_hWrite);
		m_wevRegisted = false;
		// TODO：只使用一个ev_io
		//epoll_ctl(CEnv::getEVLoop()->backend_fd, EPOLL_CTL_MOD, m_fd, m_hRead.getEv());
	}
}

void CClientConn::__appendBuffer(const char* pData, size_t uSize)
{
	if (m_strOutput.size() + uSize > MAX_OUTPUT_SIZE)
		throw exception_errno( toString("[CClientConn::%s] buffer overflow", __FUNCTION__) );

	m_strOutput.append(pData, uSize);
}

void CClientConn::__sendBuffer()
{
	if (!m_strOutput.empty())
	{
		int nSendBytes = __sendData(m_strOutput.data(), m_strOutput.size());

		// 去掉本次发送的数据
		if (nSendBytes > 0)
			m_strOutput.erase(0, nSendBytes);
	}
}

// 返回已写到fd对应的系统缓冲区的大小
size_t CClientConn::__sendData(const char* pData, size_t uSize)
{
	size_t bytes_total = 0;
	while (bytes_total < uSize)
	{
		// 外部命名空间的send函数 TOSTUDY
		int bytes_out = ::send(m_fd, (pData + bytes_total), (uSize - bytes_total), 0);
		if (bytes_out == -1)
		{
			if (errno == EINTR) // 若被中断，继续写
				continue;
			else if (errno == EAGAIN) { // 写缓冲区可能写满了
				CEnv::getEVLoop()->setHandle(&m_hWrite);
				m_wevRegisted = true;
				break;
			}
			else
				throw exception_errno( toString("[CClientConn::%s] ::send", __FUNCTION__) );
		}
		else if (bytes_out > 0)
		{
			bytes_total += (size_t)bytes_out;
		}
	}

	return bytes_total;
}

size_t CClientConn::__recvData(char* pData, size_t uSize)
{
	size_t bytes_total = 0;
	while (bytes_total < uSize)
	{
		int bytes_in = ::recv(m_fd, (pData + bytes_total), (uSize - bytes_total), 0);
		if (bytes_in == -1)
		{
			if (errno == EINTR)
				continue;
			else if (errno == EAGAIN)
				break;
			else
				throw exception_errno( toString("[CClientConn::%s] ::recv", __FUNCTION__) );
		}
		else if (bytes_in == 0)
		{
			throw exception_errno( toString("[CClientConn::%s] ::recv peer close", __FUNCTION__) );
		}
		else
		{
			bytes_total += (size_t)bytes_in;
		}
	}

	return bytes_total;
}

void CClientConn::__willFreeMyself(const std::string& strDesc)
{
	LOG(Info, "[CClientConn::%s] fd:[%d] Conn:[%p] peer:[%s:%u], %s, delete myself", __FUNCTION__, m_fd, this, m_strPeerIp.c_str(), m_uPeerPort16, strDesc.c_str());

	// delete时通过析构函数关闭连接
	delete this;
}
