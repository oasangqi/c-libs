#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <fstream>
#include <unistd.h>
#include "Client.h"
#include "Config.h"
#include "Dispatch.h"
#include "Helper.h"
#include "Types.h"
#include "Log.h"
#include "LinkEV.h"
#include "../EVWork.h"
#include "../JsonData.h"
#include "../JsonMFC.h"
#include "libtinyredis/RedisFactory.h"
#include "libtinyredis/RedisClient.h"

using namespace evwork;

static int get_options(int argc, char *argv[]);
static void __cbTimerAddUser(struct ev_loop *loop, struct ev_timer *w, int revents);
static void __cbTimerMonitor(struct ev_loop *loop, struct ev_timer *w, int revents);
static void __cbTimerDisconnect(struct ev_loop *loop, struct ev_timer *w, int revents);
static int client_start(void);
static void sigusr1_handler(int);
static bool single_instance(const char* pid_file);

struct g_args_s g_args;

ev_timer	evTimerParseCfg;
ev_timer	evTimerAddUser;
ev_timer	evTimerMonitor;
ev_timer	evTimerDisconnect;

int main(int argc, char* argv[])
{
	// 解析参数，预留
	if (get_options(argc, argv) < 0) {
		fprintf(stderr, "解析参数失败\n");
		return -1;
	}

	// 读配置
	g_args.main_db = NULL;
	if (parse_cfg() < 0) {
		fprintf(stderr, "读配置失败\n");
		return -1;
	}

	daemon(1, 0);
	srand((unsigned)time(NULL));
	loginit();

	if (!single_instance(g_args.pidfile.c_str())) {
		DBUG(ERROR, "程序已启动\n");
		return -1;
	}

	// 修改fd上限
	struct rlimit   rlim, rlim_new;
	if (getrlimit(RLIMIT_NOFILE, &rlim) == 0) {
		rlim_new.rlim_cur = rlim_new.rlim_max = RLIM_INFINITY;
		if (setrlimit(RLIMIT_NOFILE, &rlim_new) != 0) {
			rlim_new.rlim_cur = rlim_new.rlim_max = rlim.rlim_max;
			setrlimit(RLIMIT_NOFILE, &rlim_new);
		}   
	} 

	DBUG(INFO, "启动程序\n");

	// 启动
	client_start();
	logDinit();
	return 0;
}

static int get_options(int argc, char *argv[])
{
	char	*p;  
	int	i;   

	for (i = 1; i < argc; i++) {
		p = (char *) argv[i];

		if (*p++ != '-') {
			return -1;
		}    

		while (*p) {
			switch (*p++) {
				case '?': 
					// 帮助信息
					return 0;
				case 'd': 
					goto next;
				default:
					return -1;
			}
		}
next:
		continue;
	}

	return 0;
}

static void __cbTimerAddUser(struct ev_loop *loop, struct ev_timer *w, int revents)
{
	// 收到退出信号，不再构造机器人
	if (g_args.quitLater) {
		DBUG(INFO, "收到退出信号，不再构造机器人");
		ev_timer_stop(CEnv::getEVLoop()->getEvLoop(), &evTimerAddUser);
		return;
	}

	// 构造一个机器人
	for (std::map<int, struct gameSvrQueue_s>::const_iterator cit = g_args.queues.begin(); cit != g_args.queues.end(); cit++) {
		// 每个场次一次只拉1个
		for (int i = 0; i < 1; i++) {
			int uid = pickOneRichRobot(cit->first);
			if (uid  > 0) {
				new Client(uid);
			} else {
				break;
			}
		}
	}
}

static void __cbTimerMonitor(struct ev_loop *loop, struct ev_timer *w, int revents)
{
	unsigned int	nOnline = g_args.cmap.size();
	unsigned int	nLoad = 0;
	unsigned int	nKill = 0;

	for (std::map<int, struct gameSvrQueue_s>::const_iterator cit = g_args.queues.begin(); cit != g_args.queues.end(); cit++) {
		const std::vector<int> &loadQueue = cit->second.randomUids;
		const std::vector<int> &killQueue = cit->second.dels;
		std::ostringstream ostst;

		nLoad += loadQueue.size();
		nKill += killQueue.size();

		for (std::vector<int>::const_iterator cit2 = loadQueue.begin(); cit2 != loadQueue.end(); cit2++) {
			ostst << *cit2 << ",";
		}
		if (loadQueue.size() > 0) {
			DBUG(DEBUG, "vid:%d 排队机器人[%s]", cit->first, ostst.str().c_str());
		}

		std::ostringstream ostst2;
		for (std::vector<int>::const_iterator cit2 = killQueue.begin(); cit2 != killQueue.end(); cit2++) {
			ostst2 << *cit2 << ",";
		}
		if (killQueue.size() > 0) {
			DBUG(DEBUG, "vid:%d 待断开机器人[%s]", cit->first, ostst2.str().c_str());
		}
	}
	DBUG(INFO, "机器人配置个数:%u 排队个数:%u 待断开个数:%u 在线个数:%u", g_args.robots.size(), nLoad, nKill, nOnline);


	if (nOnline == 0 && g_args.quitLater ) {
		DBUG(INFO, "机器人已全部退出, 结束进程");
		exit(0);
	}
}

static void __cbTimerDisconnect(struct ev_loop *loop, struct ev_timer *w, int revents)
{
	for (std::map<int, struct gameSvrQueue_s>::iterator cit = g_args.queues.begin(); cit != g_args.queues.end(); cit++) {
		std::vector<int> &killQueue = cit->second.dels;

		// 每个场次一次只断开1个
		for (int i = 0; i < 1; i++) {
			if (killQueue.size() == 0) {
				break;
			}

			std::vector<int>::iterator it = killQueue.begin();
			evwork::IConn* pConn = g_args.conMap[*it];

			if (!pConn) {
				DBUG(INFO, "连接:%d 已断开", *it);
				killQueue.erase(it);
				continue;
			}
			DBUG(INFO, "主动断开连接:%d %p vid:%d", *it, pConn, cit->first);
			delete pConn;
		}
	}
}

static int client_start(void)
{
	signal(SIGPIPE, SIG_IGN);
	signal(SIGHUP, SIG_IGN);
	signal(SIGUSR1, sigusr1_handler);

	CSyslogReport LG;
	CEVLoop LP;
	CConnManager CM;
	CWriter WR;

	CEnv::setLogger(&LG);
	CEnv::setEVLoop(&LP);
	CEnv::setLinkEvent(&CM);
	CEnv::setConnManager(&CM);
	CEnv::setWriter(&WR);

	LP.init();

	// 超时时间
	CEnv::getEVParam().uConnTimeout = 180;

	CJsonData __DE;
	__DE.setPacketLimit(16*1024); 
	CEnv::setDataEvent(&__DE);

	CJsonMFC __MFC;
	__DE.setAppContext(&__MFC);

	CDispatch __DP;
	LinkEV	__LE;
	CM.addLE(&__LE);

	// 注册协议
	__MFC.addEntry(CDispatch::getFormEntries(), &__DP);

	// 定时重读配置
	time_t  now = time(NULL);
	evTimerParseCfg.data = NULL;
	ev_timer_init(&evTimerParseCfg, __cbTimerParseCfg, 19*60*60 - fmod(now, 24*60*60), 24*60*60);
	ev_timer_start(CEnv::getEVLoop()->getEvLoop(), &evTimerParseCfg);

	// 定时新增用户
	evTimerAddUser.data = NULL;
	ev_timer_init(&evTimerAddUser, __cbTimerAddUser, 1, 0.5);
	ev_timer_start(CEnv::getEVLoop()->getEvLoop(), &evTimerAddUser);
	
	// 监控
	evTimerMonitor.data = NULL;
	ev_timer_init(&evTimerMonitor, __cbTimerMonitor, 10, 10);
	ev_timer_start(CEnv::getEVLoop()->getEvLoop(), &evTimerMonitor);
	
	// 断开
	evTimerDisconnect.data = NULL;
	ev_timer_init(&evTimerDisconnect, __cbTimerDisconnect, 5, 1);
	ev_timer_start(CEnv::getEVLoop()->getEvLoop(), &evTimerDisconnect);
	
	LP.runLoop();
	return 0;
}

static void sigusr1_handler(int signum)
{
	g_args.quitLater = 1;
	DBUG(INFO, "收到退出信号");
	
	for (std::map<int, Client*>::const_iterator cit = g_args.cmap.begin(); cit != g_args.cmap.end(); cit++) {
		cit->second->cancelReady();
	}
}

static bool single_instance(const char* pid_file)
{
	int fd = -1;
	char buf[32];
	fd = open(pid_file, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	if (fd < 0) {
		return false;
	}
	struct flock lock;
	lock.l_type = F_WRLCK;
	lock.l_start = 0;
	lock.l_whence = SEEK_SET;
	lock.l_len = 0;
	if (fcntl(fd, F_SETLK, &lock) < 0) {
		return false;
	}
	ftruncate(fd, 0);
	pid_t pid = getpid();
	int len = snprintf(buf, 32, "%d", pid);
	write(fd, buf, len);

	return true;
}

