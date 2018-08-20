#include <stdio.h>
#include <fstream>
#include "Client.h"
#include "Helper.h"
#include "Types.h"
#include "libtinyredis/RedisFactory.h"
#include "libtinyredis/RedisClient.h"
#include "Log.h"

#define DEF_CONFIG_FILE     "conf/config.conf"                                         
extern struct g_args_s g_args;

static int load_server_info(Json::Value &conf)
{
	// data_db 用于读uid_set_key
	tinyredis::CRedisFactory data_db; 
	const Json::Value& dbInfo = conf["data-db"];

	std::string host = dbInfo["host"].asString();
	int port = dbInfo["port"].asInt();
	std::string passwd = dbInfo["pass"].asString();

	data_db.addRedis(host, port, passwd, 1000);

	g_args.gameSvrs.clear();
	for (int i = 0; i < (int)conf["games"].size(); i++) {   
		gameSvrInfo_t	svr;
		gameSvrQueue_t	svrQueue;
		const Json::Value& svrInfo = conf["games"][i];

		// 解析单个游戏服务器配置
		svr.vid = svrInfo["vid"].asInt();
		svrQueue.vid = svr.vid;
		svr.host = svrInfo["host"].asString();
		svr.port = svrInfo["port"].asInt();
		// 以下为预留配置
		svr.baseMoney = svrInfo["base_money"].asInt();
		svr.enterMoney = svrInfo["enter_money"].asInt();
		svr.leaveMoney = svrInfo["leave_money"].asInt();
		svr.fee = svrInfo["fee"].asInt();

		for (int j = 0; j < (int)svrInfo["indexs"].size(); j++) {   
			struct uidIndex_s	idx;
			const Json::Value& indexInfo = svrInfo["indexs"][j];

			idx.index = indexInfo["index"].asInt();
			std::string uid_set_key = indexInfo["uid_set_key"].asString();

			// 读机器人uid
			tinyredis::CRedisClient* pRedis = data_db.getRedis(1);
			tinyredis::CResult result(true);
			result = pRedis->command("SMEMBERS %s", uid_set_key.c_str());

			if (!result) {
				DBUG(ERROR, "读取key:%s 数据失败\n", uid_set_key.c_str());
				continue;
			}

			if (!result.isArray()) {
				DBUG(ERROR, "读取key:%s 数据不合法\n", uid_set_key.c_str());
				continue;
			}

			// 解析具体需要的值
			CHashResult hashResult;

			for (size_t i = 0; i < result.getArraySize(); i++) {
				tinyredis::CResult subResultField(false);
				subResultField = result.getSubReply(i);

				std::string strField;
				subResultField.getString(strField);
				int uid = atoi(strField.c_str());
				idx.uids.insert(uid);
			}

			svr.indexs.push_back(idx);
		}
		// 追加到全局列表
		if (g_args.gameSvrs.find(svr.vid) == g_args.gameSvrs.end()) {
			g_args.gameSvrs[svr.vid] = svr;
			g_args.queues[svrQueue.vid] = svrQueue;
		} else {
			DBUG(ERROR, "重复的游戏服务器配置 vid:%d\n", svr.vid);
		}
	}

	if (g_args.gameSvrs.size() == 0) {
		DBUG(ERROR, "未配置游戏服务器\n");
		return -1;
	}
	return 0;
}

static int load_robot_info(Json::Value &conf)
{
	for (std::map<int, struct gameSvrQueue_s>::const_iterator cit = g_args.queues.begin(); cit != g_args.queues.end(); cit++) {
		const std::vector<int> &loadQueue = cit->second.randomUids;
		std::ostringstream ostst;
		for (std::vector<int>::const_iterator cit2 = loadQueue.begin(); cit2 != loadQueue.end(); cit2++) {
			ostst << *cit2 << ",";
		}
		DBUG(DEBUG, "读配置前,vid:%d 排队机器人[%s]", cit->first, ostst.str().c_str());
	}

	// 连接redis
	delete g_args.main_db;
	g_args.main_db = new(tinyredis::CRedisFactory);
	for (int i = 0; i < (int)conf["main-db"].size(); i++) {
		const Json::Value& dbInfo = conf["main-db"][i];

		std::string host = dbInfo["host"].asString();
		int port = dbInfo["port"].asInt();
		std::string passwd = dbInfo["pass"].asString();

		g_args.main_db->addRedis(host, port, passwd, 1000);
	}   

	g_args.robots.clear();
	for (std::map<int, struct gameSvrInfo_s>::const_iterator cit = g_args.gameSvrs.begin(); cit != g_args.gameSvrs.end(); cit++) {
		const struct gameSvrInfo_s &svr = cit->second;

		for (std::vector<struct uidIndex_s>::const_iterator cit2 = svr.indexs.begin(); cit2 != svr.indexs.end(); cit2++) {
			// 读取待加入的机器人信息
			for (std::set<int>::const_iterator cit3 = cit2->uids.begin(); cit3 != cit2->uids.end(); cit3++) {
				int uid = *cit3;
				tinyredis::CRedisClient* pRedis = g_args.main_db->getRedis(uid);
				tinyredis::CResult result(true);
				result = pRedis->command("HGETALL hu:%d", uid);

				if (!result) {
					DBUG(ERROR, "读取机器人:%d 数据失败\n", uid);
					continue;
				}

				if (!result.isArray() || (result.getArraySize() == 0)) {
					DBUG(ERROR, "机器人:%d 数据不合法\n", uid);
					continue;
				}

				// 解析具体需要的值
				CHashResult hashResult;

				// 读取字段名、字段值
				for (size_t i = 0; i < result.getArraySize() - 1; i += 2) {
					tinyredis::CResult subResultField(false), subResultValue(false);
					subResultField = result.getSubReply(i);
					subResultValue = result.getSubReply(i+1);

					std::string strField, strValue;
					subResultField.getString(strField);
					subResultValue.getString(strValue);

					hashResult.addKV(strField, strValue);
				}

				struct userInfo_s	info;
				info.uid = uid;
				info.money = hashResult.getValue("money", 0);
				info.skey = hashResult.getValue("skey", "");
				//info.vid = hashResult.getValue("vid", 0);
				info.vid = cit->first; // 使用配置的游戏id
				//info.tid = hashResult.getValue("zid", 0); // 使用数据库中的房间id
				info.tid = 0; // 房间由服务器调度
				info.index = cit2->index;
				g_args.robots[uid] = info;

				// 把新增的机器人加入到待拉起队列
				if (g_args.robots_last.find(uid) == g_args.robots_last.end()) {
					g_args.queues[cit->first].randomUids.push_back(uid);
					DBUG(DEBUG, "机器人%d 加入排队 vid:%d", uid, cit->first);
				}
			}
		}
	}

	// 移除排队中的机器人
	for (std::map<int, struct gameSvrQueue_s>::iterator cit = g_args.queues.begin(); cit != g_args.queues.end(); cit++) {
		std::vector<int> &loadQueue = cit->second.randomUids;
		for (std::vector<int>::iterator it = loadQueue.begin(); it != loadQueue.end(); ) {
			if (g_args.robots.find(*it) == g_args.robots.end()) {
				DBUG(DEBUG, "机器人%d 移除排队:%d", *it, cit->first);
				it = loadQueue.erase(it); 
			} else {
				it++;
			}
		}
	}

	std::ostringstream ostst2;
	for (std::map<int, struct userInfo_s>::iterator cit = g_args.robots_last.begin(); cit != g_args.robots_last.end(); cit++) {
		ostst2 << cit->first << ",";
	}
	DBUG(DEBUG, "读配置前,旧的机器人列表[%s]", ostst2.str().c_str());

	std::ostringstream ostst3;
	for (std::map<int, struct userInfo_s>::iterator cit = g_args.robots.begin(); cit != g_args.robots.end(); cit++) {
		ostst3 << cit->first << ",";
	}
	DBUG(DEBUG, "读配置后,新的机器人列表[%s]", ostst3.str().c_str());

	for (std::map<int, struct gameSvrQueue_s>::const_iterator cit = g_args.queues.begin(); cit != g_args.queues.end(); cit++) {
		const std::vector<int> &loadQueue = cit->second.randomUids;
		std::ostringstream ostst;
		for (std::vector<int>::const_iterator cit2 = loadQueue.begin(); cit2 != loadQueue.end(); cit2++) {
			ostst << *cit2 << ",";
		}
		DBUG(DEBUG, "读配置后,vid:%d 排队机器人[%s]", cit->first, ostst.str().c_str());
	}

	// 保存本次机器人配置
	g_args.robots_last = g_args.robots;

	for (std::map<int, struct gameSvrQueue_s>::iterator cit = g_args.queues.begin(); cit != g_args.queues.end(); cit++) {
		std::vector<int> &loadQueue = cit->second.randomUids;
		std::random_shuffle(loadQueue.begin(), loadQueue.end());
	}

	return 0;
}

int parse_cfg(void)
{
	printf("读取配置文件中...\n");
	std::ifstream in(DEF_CONFIG_FILE, std::ifstream::binary);

	DBUG(DEBUG, "加载配置");
	if (!in) {
		DBUG(ERROR, "读配置文件:[%s]失败\n", DEF_CONFIG_FILE);
		return -1;
	}

	Json::Reader reader;
	Json::Value conf; 
	if (!reader.parse(in, conf)) {
		DBUG(ERROR, "配置文件:[%s]格式错误\n", DEF_CONFIG_FILE);
		goto fail_exit;
	}

	g_args.pidfile = conf["pid_file"].asString();
	g_args.logdir = conf["log_path"].asString();

	printf("读取游戏服务器配置中...\n");
	if (load_server_info(conf) < 0) {
		DBUG(ERROR, "配置文件:[%s]游戏服务器配置不合法\n", DEF_CONFIG_FILE);
		goto fail_exit;
	}

	printf("读取机器人信息中...\n");
	if (load_robot_info(conf) < 0) {
		DBUG(ERROR, "加载机器人信息失败");
		goto fail_exit;
	}

	printf("读取配置完成，启动程序\n");
	in.close();
	return 0;
fail_exit:
	in.close();
	return -1;
} 

void __cbTimerParseCfg(struct ev_loop *loop, struct ev_timer *w, int revents)
{
	parse_cfg();
}

int pickOneRichRobot(int vid)
{
	if (vid <= 0) {
		DBUG(ERROR, "无效的 vid:%d", vid);
		return -1;
	}

	gameSvrQueue_t	&svrQueue = g_args.queues[vid];
	if (svrQueue.vid != vid) {
		DBUG(ERROR, "未找到队列 vid:%d", vid);
		return -1;
	}

	std::vector<int> poorRobots;
	std::vector<int>::iterator it = svrQueue.randomUids.begin();

	while (it != svrQueue.randomUids.end()) {
		int uid = *it;
		tinyredis::CRedisClient* pRedis = g_args.main_db->getRedis(uid);
		tinyredis::CResult result(true);
		result = pRedis->command("HGET hu:%d money", uid);

		if (!result || !result.isString()) {
			DBUG(ERROR, "读取机器人:%d 数据失败\n", uid);
			poorRobots.push_back(uid);
			it = svrQueue.randomUids.erase(it);
			continue;
		}

		std::string str;
		result.getString(str);
		int money = atoi(str.c_str());
		gameSvrInfo_t	&svr = g_args.gameSvrs[g_args.robots[uid].vid];

		if (money >= svr.enterMoney) {
			break;
		}
		poorRobots.push_back(uid);
		DBUG(INFO, "机器人:%d 金币:%d 不足, enter_money:%d\n", uid, money, svr.enterMoney);
		it = svrQueue.randomUids.erase(it);
	}

	int uid = -1;
	if (it != svrQueue.randomUids.end()) {
		uid = *it;
	}
	std::copy(poorRobots.begin(), poorRobots.end(), std::back_inserter(svrQueue.randomUids));
	return uid;
}
