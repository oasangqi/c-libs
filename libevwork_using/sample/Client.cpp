#include <algorithm>
#include "Client.h"
#include "Log.h"
#include "protocol.h"
#include "../JsonPacket.h"
#include "Helper.h"

using namespace evwork;

extern struct g_args_s	g_args;

static const char *log_level_map[] = {"ERROR", "INFO", "DEBUG"};

static const char *action_map[] = {"过", "吃", "碰", "偎", "跑", "提", "胡"};
//static const char *eattype_map[] = {"吃", "碰", "偎", "臭偎", "跑", "提"};

#define sendData(data, len) \
	do {	\
		evwork::IConn* pConn = g_args.conMap[m_fd];	\
		if (pConn) {	\
			pConn->sendBin(data, len);	\
		}	\
	} while (0)

enum _Action_code
{   
	ACTION_CHI      = 1,
	ACTION_PENG     ,   
	ACTION_WEI      ,   
	ACTION_PAO      ,   
	ACTION_TI       ,   
	ACTION_HU       ,   
	ACTION_END      , // 初始值，表示没做出选择
	ACTION_PASS     =0, 
};

Client::Client(int uid)
	:m_uid(uid),
	m_playing(false)
{
	open_log();
	_startTimers();

	int vid = g_args.robots[m_uid].vid;
	std::vector<int> &loadQueue = g_args.queues[vid].randomUids;
	loadQueue.erase(loadQueue.begin());
	gameSvrInfo_t	&svr = g_args.gameSvrs[g_args.robots[m_uid].vid];

	m_pConn = CEnv::getConnManager()->connectServer(svr.host, svr.port);
	if (!m_pConn) {
		DBUG(ERROR, "机器人:%d 构造完成,连接服务器失败 vid:%d 排队数:%d\n", m_uid, vid, loadQueue.size());
		delete this;
	}

	m_fd = m_pConn->getfd();
	if (g_args.cmap.find(m_fd) != g_args.cmap.end()) {
		Client *pClient = g_args.cmap[m_fd];
		DBUG(ERROR, "与机器人:%d fd重复:%d", pClient->m_uid, m_fd);
	}

	g_args.cmap[m_fd] = this;
	DBUG(INFO, "机器人:%d 构造完成, fd:%d vid:%d 排队数:%d\n", m_uid, m_fd, vid, loadQueue.size());
}

Client::~Client()
{
	if (m_fd > 0) {
		g_args.cmap.erase(m_fd);
	}

	if (g_args.robots.find(m_uid) != g_args.robots.end()) {
		int vid = g_args.robots[m_uid].vid;
		std::vector<int> &loadQueue = g_args.queues[vid].randomUids;
		loadQueue.push_back(m_uid);
		DBUG(INFO, "机器人:%d 析构后排队 vid:%d\n", m_uid, vid);
	} else {
		DBUG(INFO, "机器人:%d 析构后不再排队\n", m_uid);
	}

	_stopTimers();

	close_log();

	DBUG(INFO, "机器人:%d 析构完成\n", m_uid);
}

void Client::__doLogin()
{
	log(INFO, "登录");

	// 连接失败(服务器繁忙或是根本连不上)
	if (g_args.conMap.find(m_fd) == g_args.conMap.end()) {
		DBUG(DEBUG, "连接服务器失败");

		// 避免CClientconn对象内存泄露，通过全局方法清理
		if (g_args.conMap.find(m_fd) != g_args.conMap.end()) {
			Client *pClient = g_args.cmap[m_fd];
			DBUG(ERROR, "%s 与机器人:%d fd重复:%d", __FUNCTION__, pClient->m_uid, m_fd);
		}
		g_args.conMap[m_fd] = m_pConn;
		__disConnectLater();
		return;
	}

	// 进入房间
	Jpacket packet;
	packet.val["cmd"] = CLIENT_LOGIN;
	packet.val["uid"] = m_uid;
	packet.val["skey"] = g_args.robots[m_uid].skey;
	packet.val["room"] = g_args.robots[m_uid].tid;
	packet.val["index"] = g_args.robots[m_uid].index;
	packet.end();
	sendData(packet.tostring().c_str(), packet.tostring().size());

	DBUG(INFO, "机器人:%d 请求进入游戏:%d 房间:%d index:%d fd:%d\n", m_uid, g_args.robots[m_uid].vid, 
			g_args.robots[m_uid].tid, g_args.robots[m_uid].index, m_fd);

}

void Client::__beReady()
{
	log(INFO, "准备");
	Jpacket packet;
	packet.val["cmd"] = CLIENT_READY;
	packet.end();

	sendData(packet.tostring().c_str(), packet.tostring().size());
}

void Client::__doHeartBeat()
{
	Jpacket packet;
	packet.val["cmd"] = SYSTEM_ECHO;
	packet.end();

	sendData(packet.tostring().c_str(), packet.tostring().size());
}

void Client::__doAction()
{
	// TODO:根据packetAction发送决策
	Json::Value &val = packetAction.tojson();
	std::string	str;
	std::vector<int> actions;
	int	choose;
	int chiIdx, biIdx;

	for (unsigned int i = 0; i < val["actionList"].size(); i++) {
		int	action = val["actionList"][i].asInt();
		actions.push_back(action);
		str += action_map[action];
		str += ",";
	}
	log(INFO, "收到选项[%s]", str.c_str());

	if (std::find(actions.begin(), actions.end(), ACTION_HU) != actions.end()) {
		choose = ACTION_HU;
	} else if (find(actions.begin(), actions.end(), ACTION_PENG) != actions.end()) {
		choose = ACTION_PENG;
	} else if (find(actions.begin(), actions.end(), ACTION_CHI) != actions.end()) {
		if (__wiseEat(val, chiIdx, biIdx)) {
			choose = ACTION_CHI;
		} else {
			choose = ACTION_PASS;
		}
	} else {
		choose = ACTION_PASS;
	}

	log(INFO, "选择[%s]", action_map[choose]);

	Jpacket packetr;
	packetr.val["cmd"] = CLIENT_GAME_ACTION;
	packetr.val["action"] = choose;
	packetr.val["sn"] = val["sn"].asInt();
	if (choose == ACTION_CHI) {
		// 吃、比ID
		packetr.val["params"].append(chiIdx);
		if (biIdx >= 0) {
			packetr.val["params"].append(biIdx);
		}
	}
	packetr.end();

	sendData(packetr.tostring().c_str(), packetr.tostring().size());
}

void Client::__outCard()
{
	// TODO:出牌策略
	HANDCARDS_t	vcards = handcards.getHandCards();
	Card cardval;
	for (HANDCARDS_t::const_iterator ito = vcards.begin(); ito != vcards.end(); ito++) {
		if (ito->size() >= 3 && ito->front() != ito->back() && ito->front() != 27) {
			cardval = ito->front();
			break;
		}
	}

	for (HANDCARDS_t::const_iterator ito = vcards.begin(); ito != vcards.end(); ito++) {
		if (ito->size() == 2 && ito->front() == ito->back() && ito->front() != 27) {
			cardval = ito->front();
			break;
		}
	}

	for (HANDCARDS_t::const_iterator ito = vcards.begin();
			ito != vcards.end(); ito++) {
		if (ito->size() == 2 && ito->front() != ito->back() && ito->front() != 27) {
			cardval = ito->front();
			break;
		}
	}

	for (HANDCARDS_t::const_iterator ito = vcards.begin();
			ito != vcards.end(); ito++) {
		if (ito->size() == 1 && ito->front() != 27) {
			cardval = ito->front();
			break;
		}
	}

	if (cardval == 0) {
		log(INFO, "打不出牌,出BUG了.");
		return;
	}
	log(INFO, "从手里打出牌[%d]", cardval.getVal());

	Jpacket packetr;
	packetr.val["cmd"] = CLIENT_GAME_OUTCARD;
	packetr.val["card"] = cardval.getVal();
	packetr.end();
	sendData(packetr.tostring().c_str(), packetr.tostring().size());
}

bool Client::__wiseEat(Json::Value& val, int& chiIdx, int& biIdx)
{
	std::map< int, int > chiXiMP;
	std::map< int, std::map<int, int> > biXiMP;
	Card card;

	if (val["params"].size() == 0) {
		return false;
	}
	if (val["params"].size() >= 1) {
		Json::Value &chiArray = val["params"][0];
		for (unsigned int i = 0; i < chiArray.size(); i++) {
			int	cardval = chiArray[i]["card"].asInt();
			int	idx = chiArray[i]["idx"].asInt();

			int sum = 0;
			int xi = 0;
			for (unsigned int j = 0; j < chiArray[i]["handCards"].size(); j++) {
				sum += chiArray[i]["handCards"][j].asInt();
			}

			// 只吃1、2、3、7、10
			if (!Card(cardval).IsXiCard()) {
				log(INFO, "牌:%d 吃后没有息", cardval);
				continue;
			}

			card = Card(cardval);
			if (sum == 6 || (sum == 19 && card.getPoint() != 1)) {
				xi = 3;
			} else if (sum == 54 || sum == 67) {
				xi = 6;
			}

			log(INFO, "吃牌:%d idx:%d sum:%d xi:%d", cardval, idx, sum, xi);
			chiXiMP[idx] = xi;
		}
	}
	if (val["params"].size() > 1) {
		Json::Value &biArray = val["params"][1];
		int lastIdx = -1;
		int offset = 0;
		for (unsigned int i = 0; i < biArray.size(); i++) {
			int	idx = biArray[i]["idx"].asInt();
			if (idx == lastIdx) {
				offset++;
			} else {
				lastIdx = idx;
				offset = 0;
			}


			int sum1 = 0;
			int sum2 = 0;
			int	xi = 0;

			for (unsigned int j = 0; j < biArray[i]["hand1"].size(); j++) {
				sum1 += biArray[i]["hand1"][j].asInt();
			}
			for (unsigned int j = 0; j < biArray[i]["hand2"].size(); j++) {
				sum2 += biArray[i]["hand2"][j].asInt();
			}

			// 去掉 1 1 壹
			if (sum1 == 6 || (sum1 == 19 && card.getPoint() != 1)) {
				xi += 3;
			} else if (sum1 == 54 || sum1 == 67) {
				xi += 6;
			}
			if (sum2 == 6 || (sum2 == 19 && card.getPoint() != 1)) {
				xi += 3;
			} else if (sum2 == 54 || sum2 == 67) {
				xi += 6;
			}


			std::map<int, int>& ref = biXiMP[idx]; // 第idx种吃法对应的比法集合
			ref[offset] = xi; // 第idx种吃法对应的第offset种比法

			log(INFO, "比牌 idx:%d i:%d xi:%d sum1:%d sum2:%d", idx, i, xi, sum1, sum2);
		}
	}

	//------------------
	for (std::map<int, int>::const_iterator cit = chiXiMP.begin(); cit != chiXiMP.end(); cit++) {
		std::map<int, int> &tmp = biXiMP[cit->first];
		log(DEBUG, "chiIdx:%d chiXi:%d ", cit->first, cit->second);
		for (std::map<int, int>::const_iterator bit = tmp.begin(); bit != tmp.end(); bit++) {
			log(DEBUG, "chiIdx:%d chiXi:%d biIdx:%d biXi:%d", cit->first, cit->second, bit->first, bit->second);
		}
	}
	//------------------
	
	// 选最优的吃idx
	int maxXi = 0;
	int chiBest = -1;
	for (std::map<int, int>::const_iterator cit = chiXiMP.begin(); cit != chiXiMP.end(); cit++) {
		int max = cit->second;

		std::map<int, int> &tmp = biXiMP[cit->first];
		int maxBiXi = 0;
		for (std::map<int, int>::const_iterator bit = tmp.begin(); bit != tmp.end(); bit++) {
			if (bit->second >= maxBiXi) {
				maxBiXi = bit->second;
			}
		}
		if (max + maxBiXi >= maxXi) {
			maxXi = max + maxBiXi;
			chiBest = cit->first;
		}
	}

	log(INFO, "maxXi:%d chiBest:%d ", maxXi, chiBest);
	if (maxXi <= 0 || chiBest < 0) {
		return false;
	}

	// 选最优的比idx
	maxXi = 0;
	int biBest = -1;

	std::map<int, int> &tmp = biXiMP[chiBest];
	for (std::map<int, int>::const_iterator bit = tmp.begin(); bit != tmp.end(); bit++) {
		if (bit->second >= maxXi) {
			maxXi = bit->second;
			biBest = bit->first;
		}
	}
	log(INFO, "maxXi:%d biBest:%d ", maxXi, biBest);

	chiIdx = chiBest;
	biIdx = biBest;

	return true;
}

void Client::__doLogout()
{
	log(INFO, "退出房间");
	Jpacket packetr;
	packetr.val["cmd"] = CLIENT_LOGOUT;
	packetr.end();
	sendData(packetr.tostring().c_str(), packetr.tostring().size());
}

void Client::__disConnectLater()
{
	//ev_timer_stop(CEnv::getEVLoop()->getEvLoop(), &m_evTimerHeartBeat);
	int vid = g_args.robots[m_uid].vid;
	std::vector<int> &killQueue = g_args.queues[vid].dels;
	DBUG(DEBUG, "机器人:%d fd:%d vid:%d 等待断开", m_uid, m_fd, vid);

	if (std::find(killQueue.begin(), killQueue.end(), m_fd) != killQueue.end()) {
		DBUG(ERROR, "待断开队列异常,重复的fd:%d ", m_fd);
		return;
	}
	killQueue.push_back(m_fd);
}

void Client::__dumpHandCards()
{
	std::ostringstream ostst;
	handcards.dumpHandsCards(ostst);
	log(INFO, "手牌[%s]", ostst.str().c_str());
}

void Client::readyLater()
{
	int start = rand() % 6 + 3;

	ev_timer_stop(CEnv::getEVLoop()->getEvLoop(), &m_evTimerReady);
	ev_timer_init(&m_evTimerReady, Client::__cbTimerReady, start, 0);
	ev_timer_start(CEnv::getEVLoop()->getEvLoop(), &m_evTimerReady);

	// 进入房间成功之后才发心跳
	ev_timer_start(CEnv::getEVLoop()->getEvLoop(), &m_evTimerHeartBeat);
}

void Client::__logoutLater(int low, int up)
{
	int start = rand() % (up - low + 1) + low;

	ev_timer_stop(CEnv::getEVLoop()->getEvLoop(), &m_evTimerLogout);
	ev_timer_init(&m_evTimerLogout, Client::__cbTimerLogout, start, 0);
	ev_timer_start(CEnv::getEVLoop()->getEvLoop(), &m_evTimerLogout);
}

void Client::cancelReady()
{
	if (m_playing) {
		// 正在玩牌的机器人打完后再退出
		return;
	}

	log(INFO, "取消准备");
	m_playing = false;

	Jpacket packetr;
	packetr.val["cmd"] = CLIENT_LOGOUT;
	packetr.end();
	sendData(packetr.tostring().c_str(), packetr.tostring().size());

	// 制造被动断开
	__disConnectLater();
}

void Client::cbAction(Jpacket& packet)
{
	int start = rand() % 4 + 1;

	packetAction = packet;

	ev_timer_stop(CEnv::getEVLoop()->getEvLoop(), &m_evTimerAction);
	ev_timer_init(&m_evTimerAction, Client::__cbTimerAction, start, 0);
	ev_timer_start(CEnv::getEVLoop()->getEvLoop(), &m_evTimerAction);

}

void Client::cbEnterRoom(Jpacket& packet)
{
	Json::Value &val = packet.tojson();

	log(INFO, "进入房间");

	if (val["code"].asInt() != 0) {
		log(ERROR, "进入房间失败:[%d] ", val["code"].asInt());
		// 进入房间失败不发心跳，由服务器超时断开
		return;
	}

	// 获取房间信息
	Jpacket packetr;
	packetr.val["cmd"] = CLIENT_TABLE_INFO;
	packetr.end();
	sendData(packetr.tostring().c_str(), packetr.tostring().size());

	// 延迟准备
	readyLater();
}

void Client::cbEnterRoomFail(Jpacket& packet)
{
	Json::Value &val = packet.tojson();

	log(ERROR, "进入房间失败:[%d] ", val["code"].asInt());

	__disConnectLater();
}

void Client::cbChangeTableFail(Jpacket& packet)
{
	Json::Value &val = packet.tojson();

	log(ERROR, "换房失败:[%d] ", val["type"].asInt());

	__logoutLater(1,4);
}

void Client::cbReady(Jpacket& packet)
{
	Json::Value &val = packet.tojson();

	if (val["uid"].asInt() == m_uid) {
		log(INFO, "准备成功");
		__logoutLater(30, 60);
	} else {
		log(INFO, "玩家[%d]准备成功", val["uid"].asInt());
	}
}

void Client::cbReadyFail(Jpacket& packet)
{
	Json::Value &val = packet.tojson();

	log(ERROR, "准备失败:[%s] ", val["msg"].asString().c_str());

	__logoutLater(1,4);
}

void Client::cbInitHandCards(Jpacket& packet)
{
	Json::Value &val = packet.tojson();
	m_seatid = val["seatid"].asInt();

	log(INFO, "初始化手牌");
	m_playing = true;
	// 游戏开始，关闭等待超时退出房间的定时器
	ev_timer_stop(CEnv::getEVLoop()->getEvLoop(), &m_evTimerLogout);

	handcards.clear();
	for (unsigned int i = 0; i < val["cards"].size(); i++) {
		for (unsigned int j = 0; j < val["cards"][i].size(); j++) {
			int	cardval = val["cards"][i][j].asInt();
			// 放到手牌
			handcards.addCard(cardval);
		}
	}
	__dumpHandCards();
}

void Client::cbTableInfo(Jpacket& packet)
{
	Json::Value &val = packet.tojson();
	std::ostringstream ostst;
	m_seatid = val["player_seatid"].asInt();
	int tid = val["tid"].asInt();

	log(INFO, "房间[%d] 座位[%d]", tid, m_seatid);

	handcards.clear();
	for (unsigned int i = 0; i < val["hole_cards"].size(); i++) {
		for (unsigned int j = 0; j < val["hole_cards"][i].size(); j++) {
			int	cardval = val["hole_cards"][i][j].asInt();
			handcards.addCard(cardval);
		}
	}
	__dumpHandCards();

	for (unsigned int i = 0; i < val["players"].size(); i++) {
		int	uid = val["players"][i]["uid"].asInt();
		int	istrust = val["players"][i]["trust"].asInt();

		// 取消托管
		if ( uid == m_uid && istrust) {
			Jpacket packetr;
			packetr.val["cmd"] = CLIENT_CANCEL_TRUST;
			packetr.end();
			sendData(packetr.tostring().c_str(), packetr.tostring().size());
			break;
		}
	}
#if 0
	std::ostringstream ostste;
	for (unsigned int i = 0; i < val["players"].size(); i++) {
		if (val["players"][i]["seatid"] != m_seatid) {
			continue;
		}

		for (unsigned int j = 0; j < val["players"][i]["eatCards"].size(); j++) {
			ostste << "(";
			int	cardval = val["players"][i]["eatCards"][j]["card"].asInt();
			int	type = val["players"][i]["eatCards"][j]["type"].asInt();
			ostste << cardval << "," << eattype_map[type];
			ostste << ") ";
		}
	}
	log(INFO, "\t吃牌[%s]", ostste.str().c_str());
#endif
}

void Client::cbNextRound(Jpacket& packet)
{
	Json::Value &val = packet.tojson();

	int banker_seatid = val["banker_seatid"].asInt();
	int hu_seatid = val["hu_seatid"].asInt();

	log(INFO, "小结算, 庄家座位[%d], 胡牌人座位[%d]", banker_seatid, hu_seatid);
	m_playing = false;

	// 收到退出信号,退出房间
	if (g_args.quitLater == 1 || willChangeTable()) {
		__logoutLater(1,4);
		return;
	}

	// 概率换房
#if 0
	if (willChangeTable()) {
		log(INFO, "换房间");
		Jpacket packetr;
		packetr.val["cmd"] = CLIENT_CHANGE_TABLE;
		packetr.end();
		sendData(packetr.tostring().c_str(), packetr.tostring().size());
		return;
	} 
#endif


	// 延迟准备
	log(INFO, "延迟准备");
	readyLater();
}

void Client::cbLogoutOK(Jpacket& packet)
{
	Json::Value &val = packet.tojson();

	if (val["uid"].asInt() == m_uid) {
		log(INFO, "退出房间成功");
		__disConnectLater();
	}
}

void Client::cbSomeOneEat(Jpacket& packet)
{
	std::ostringstream ostst;
	Json::Value &val = packet.tojson();

	int	seatid = val["seatid"].asInt();
	int	action = val["action"].asInt();

	log(INFO, "有人进牌");

	if (m_seatid == seatid) {
		log(INFO, "自己操作[%s] ", action_map[action]);
	} else {
		// 停止自己的操作选项
		ev_timer_stop(CEnv::getEVLoop()->getEvLoop(), &m_evTimerAction);
		log(INFO, "玩家座位[%d] 操作[%s] ", seatid, action_map[action]);
	}

	int cardval;
	for (unsigned int i = 0; i < val["eatCards"].size(); i++) {
		cardval = val["eatCards"][i]["card"].asInt();
		ostst << cardval << ",";
	}
	log(INFO, "\t操作的牌[%s]", ostst.str().c_str());

	if (m_seatid == seatid)
	{
		if (action == ACTION_PENG || action == ACTION_WEI) {
			handcards.removeCard(cardval, 2);
		} else if (action == ACTION_PAO || action == ACTION_TI) {
			handcards.removeCard(cardval, 4);
		} else if (action == ACTION_CHI) {
			handcards.removeCard(cardval, -1);
			for (unsigned int i = 0; i < val["eatCards"].size(); i++) {
				for (unsigned int j = 0; j < val["eatCards"][i]["cards"].size(); j++) {
					handcards.removeCard(val["eatCards"][i]["cards"][j].asInt(), 1);
				}
			}
		}
		__dumpHandCards();
	}
}

void Client::cbNoticeOutCard(Jpacket& packet)
{
	std::ostringstream ostst;
	std::string	str;
	Json::Value &val = packet.tojson();

	log(INFO, "提示出牌");

	int	seatid = val["seatid"].asInt();
	if (seatid != m_seatid) {
		log(INFO, "等待玩家出牌, 座位[%d]", seatid);
		return;
	}

	int start = rand() % 2 + 1;

	ev_timer_stop(CEnv::getEVLoop()->getEvLoop(), &m_evTimerOutCard);
	ev_timer_init(&m_evTimerOutCard, Client::__cbTimerOutCard, start, 0);
	ev_timer_start(CEnv::getEVLoop()->getEvLoop(), &m_evTimerOutCard);
}

void Client::cbSomeOneOutCard(Jpacket& packet)
{
	Json::Value &val = packet.tojson();

	int seatid = val["seatid"].asInt();
	int card = val["card"].asInt();
	int fetch = val["fetch"].asInt();

	log(INFO, "玩家出牌");

	if (fetch == 1) {
		if (seatid != m_seatid) {
			log(INFO, "玩家座位[%d], 摸出牌[%d]", seatid, card);
		} else {
			log(INFO, "摸出牌[%d]", card);
		}
	} else {
		if (seatid != m_seatid) {
			log(INFO, "玩家座位[%d], 从手中打出牌[%d]", seatid, card);
		} else {
			log(INFO, "从手中打出牌[%d]", card);
			handcards.outHandCard(card);
			__dumpHandCards();
		}
	}
}

void Client::cbOutCardOK(Jpacket& packet)
{
	//evwork::IConn* pConn = g_args.conMap[m_fd];
	Json::Value &val = packet.tojson();

	log(INFO, "出牌结果");

	int seat = val["seatid"].asInt();
	int cardval = val["card"].asInt();
	if (seat == m_seatid)
	{
		// 停止出牌定时器，因为可能是服务器主动替玩家出的牌
		ev_timer_stop(CEnv::getEVLoop()->getEvLoop(), &m_evTimerOutCard);

		log(INFO, "没人要牌[%d],出牌成功", cardval);
		if (val["fetch"].isInt() && val["fetch"].asInt() == 0) {
			// 从手里去掉这张牌
			// 已通过4023去掉了这张打出的牌
			//handcards.outHandCard(cardval);
		}
	}
}

void Client::cbSyncHandCards(Jpacket& packet)
{
	Json::Value &val = packet.tojson();
	std::ostringstream ostst;
	m_seatid = val["player_seatid"].asInt();

	log(INFO, "同步手牌");

	handcards.clear();
	for (unsigned int i = 0; i < val["cards"].size(); i++) {
		for (unsigned int j = 0; j < val["cards"][i].size(); j++) {
			int	cardval = val["cards"][i][j].asInt();
			handcards.addCard(cardval);
		}
	}
	__dumpHandCards();
}

void Client::__cbTimerLogin(struct ev_loop *loop, struct ev_timer *w, int revents)
{
	Client* pThis = (Client*)w->data;
	pThis->__doLogin();
}

void Client::__cbTimerLogout(struct ev_loop *loop, struct ev_timer *w, int revents)
{
	Client* pThis = (Client*)w->data;
	pThis->__doLogout();
}

void Client::__cbTimerHeartBeat(struct ev_loop *loop, struct ev_timer *w, int revents)
{
	Client* pThis = (Client*)w->data;
	pThis->__doHeartBeat();
}

void Client::__cbTimerReady(struct ev_loop *loop, struct ev_timer *w, int revents)
{
	Client* pThis = (Client*)w->data;
	pThis->__beReady();
}

void Client::__cbTimerAction(struct ev_loop *loop, struct ev_timer *w, int revents)
{
	Client* pThis = (Client*)w->data;
	pThis->__doAction();
}

void Client::__cbTimerOutCard(struct ev_loop *loop, struct ev_timer *w, int revents)
{
	Client* pThis = (Client*)w->data;
	pThis->__outCard();
}

Client *findClientByfd(int fd)
{
	std::map<int, Client*>::iterator iter = g_args.cmap.find(fd);
	if (iter != g_args.cmap.end()) {
		return iter->second;
	}
	return NULL;
}

void Client::open_log()
{
	char	path[64] = "\0";
	snprintf(path, 64, "%s/%d", g_args.logdir.c_str(), m_uid);
	m_logfd = open(path, O_RDWR | O_CREAT | O_APPEND, 0666);
}

void Client::close_log()
{
	close(m_logfd);
}

void Client::log(int level, const char *fmt, ...)
{
	time_t  iTime = time(NULL);
	struct tm* pTM = localtime(&iTime);
	va_list     ap; 

	memset(m_logbuf, 0, LOG_BUFF_SIZE);
	memset(m_msgbuf, 0, MSG_BUFF_SIZE);

	va_start(ap, fmt);
	vsnprintf(m_msgbuf, MSG_BUFF_SIZE, fmt, ap);
	va_end(ap);

	snprintf(m_logbuf, LOG_BUFF_SIZE, "[%04d-%02d-%02d %02d:%02d:%02d %s] %s\n", pTM->tm_year + 1900, pTM->tm_mon + 1,
			pTM->tm_mday, pTM->tm_hour, pTM->tm_min, pTM->tm_sec, log_level_map[level], m_msgbuf);
	write(m_logfd, m_logbuf, strlen(m_logbuf));
	//fsync(m_logfd);
}

void Client::_startTimers()
{
	// 延迟登录
	m_evTimerLogin.data = this;
	ev_timer_init(&m_evTimerLogin, Client::__cbTimerLogin, 2, 0);
	ev_timer_start(CEnv::getEVLoop()->getEvLoop(), &m_evTimerLogin);

	m_evTimerLogout.data = this;
	ev_timer_init(&m_evTimerLogout, Client::__cbTimerLogout, 2, 0);

	m_evTimerHeartBeat.data = this;
	ev_timer_init(&m_evTimerHeartBeat, Client::__cbTimerHeartBeat, 20, 20);

	m_evTimerReady.data = this;
	ev_timer_init(&m_evTimerReady, Client::__cbTimerReady, 3, 0);

	m_evTimerAction.data = this;
	ev_timer_init(&m_evTimerAction, Client::__cbTimerAction, 3, 0);

	m_evTimerOutCard.data = this;
	ev_timer_init(&m_evTimerOutCard, Client::__cbTimerOutCard, 3, 0);
}

void Client::_stopTimers()
{
	ev_timer_stop(CEnv::getEVLoop()->getEvLoop(), &m_evTimerOutCard);
	ev_timer_stop(CEnv::getEVLoop()->getEvLoop(), &m_evTimerAction);
	ev_timer_stop(CEnv::getEVLoop()->getEvLoop(), &m_evTimerReady);
	ev_timer_stop(CEnv::getEVLoop()->getEvLoop(), &m_evTimerHeartBeat);
	ev_timer_stop(CEnv::getEVLoop()->getEvLoop(), &m_evTimerLogout);
	ev_timer_stop(CEnv::getEVLoop()->getEvLoop(), &m_evTimerLogin);
}

