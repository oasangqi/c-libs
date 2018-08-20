#pragma once 

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <iostream>
#include <sstream>
#include <string>
#include <set>

#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#include <ev.h>

#include "ExceptionErrno.h"
#include "FuncHelper.h"

namespace evwork
{

	struct IConn
	{
	public:
		IConn() : m_fd(-1), m_cid(0) {}
		virtual ~IConn() {}

		void setcid(uint32_t cid) { m_cid = cid; }
		uint32_t getcid() { return m_cid; }
		uint32_t getfd() { return m_fd; }

		virtual void getPeerInfo(std::string& strPeerIp, uint16_t& uPeerPort16) = 0;
		virtual bool sendBin(const char* pData, size_t uSize) = 0;

	protected:
		int m_fd;
		uint32_t m_cid;
	};

	struct ILinkEvent
	{
	public:
		virtual ~ILinkEvent() {}

		virtual void onConnected(IConn* pConn) = 0;
		virtual void onClose(IConn* pConn) = 0;
	};

	struct IDataEvent
	{
	public:
		virtual ~IDataEvent() {}

		virtual int onData(IConn* pConn, const char* pData, size_t uSize) = 0;
	};

	struct IHandle
	{
	public:
		// 为ev_io结构绑定了调用回调函数执行时所需参数
		// 这个this是子类还是父类的指针(应该是子类)  TOSTUDY
		IHandle() : m_fd(-1), m_ev(0) { m_evio.data = this; }
		virtual ~IHandle() {}

		void setFd(int fd) { m_fd = fd; }
		int getFd() { return m_fd; }

		void setEv(int ev) { m_ev = ev; }
		int getEv() { return m_ev; }

		ev_io& getEvIo() { return m_evio; }

		virtual void cbEvent(int revents) = 0;

		static void evCallBack(struct ev_loop *loop, struct ev_io *w, int revents)
		{
			IHandle* pThis = (IHandle*)w->data;

			pThis->cbEvent(revents);
		}

	protected:
		int m_fd;
		int m_ev;
		ev_io m_evio;
	};

	template <typename T, void (T::*fn)(int revents)>
	class THandle
		: public IHandle
	{
	public:
		THandle(T* p) : m_pObj(p) {}
		virtual ~THandle() {}

		void cbEvent(int revents)
		{
			(m_pObj->*fn)(revents);
		}

	protected:
		T* m_pObj;
	};

	class CEVLoop
	{
	public:
		CEVLoop();
		virtual ~CEVLoop();

		bool init();
		void destroy();

		void runLoop();

		void setHandle(IHandle* p);
		void delHandle(IHandle* p);

		struct ev_loop* getEvLoop();

	private:
		struct ev_loop* m_pEVLoop;

		std::set<IHandle*> m_setHandle;
	};

	struct IWriter
	{
	public:
		virtual ~IWriter() {}

		virtual int send(const std::string& strIp, uint16_t uPort, const char* pData, size_t uSize) = 0;
	};

	struct IConnManager
	{
	public:
		virtual ~IConnManager() {}

		virtual IConn* getConnById(uint32_t uConnId) = 0;
		virtual IConn* connectServer(const std::string& strIp, uint16_t uPort) = 0;
	};

}
