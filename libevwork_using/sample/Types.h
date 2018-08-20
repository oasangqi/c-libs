#ifndef _TYPES_H_
#define _TYPES_H_

#include <set>
#include "libtinyredis/RedisFactory.h"
#include "libevwork/EVWork.h"

#define MSG_BUFF_SIZE 2000
#define LOG_BUFF_SIZE 2048

#define unlikely(x)  __builtin_expect(!!(x), 0) 

enum logLevel {
	ERROR = 0,
	INFO,
	DEBUG
};

// 每个场次的指定index进入哪些机器人
typedef struct uidIndex_s {
	int	index;
	std::set<int>	uids;
} uidIndex_t;

// 从配置文件读出的游戏服务器信息
typedef struct gameSvrInfo_s {
	int			vid;
	std::string	host;
	int			port;
	int			baseMoney;
	int			enterMoney;
	int			leaveMoney;
	int			fee;
	std::vector<struct uidIndex_s> indexs; 
} gameSvrInfo_t;

// 每个游戏场次对应的机器人队列
typedef struct gameSvrQueue_s{
	int			vid;
	std::vector<int> randomUids; // 未启动的机器人队列，初始为随机顺序的的机器人uid
	std::vector<int>	dels; // 将要断开的连接fd
} gameSvrQueue_t; 

class Client;
// 全局信息
struct g_args_s {
	tinyredis::CRedisFactory *main_db; // 机器人信息数据库
	std::map<int, struct gameSvrInfo_s> gameSvrs; // 游戏服务器，key为vid
	std::map<int, struct userInfo_s> robots_last; // 上一次读取的机器人信息，key为uid
	std::map<int, struct userInfo_s> robots; // 机器人信息，key为uid
	std::map<int, struct gameSvrQueue_s> queues; // 每次场次待拉起、断开队列，key为vid

	std::map<int, Client*>	cmap; // 在线(不一定通过认证)的机器人，key为连接fd
	std::map<int, evwork::IConn*>	conMap; // 用于通过fd找到连接
	std::string	pidfile;
	std::string	logdir;
	int	quitLater; // 打完退出
};

// 玩家信息
struct userInfo_s {
	int	uid;
	int	money;
	std::string	skey;
	int	vid;
	int	tid;
	int	index;
};

#endif
