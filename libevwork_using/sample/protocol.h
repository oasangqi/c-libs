// 协议定义

#ifndef _PROTOCOL_H_
#define _PROTOCOL_H_

#define SYSTEM_ECHO 				1 // 当心跳处理

// 通用协议
#define CLIENT_LOGIN			1001 // 登录
#define CLIENT_LOGOUT			1002 // 登出
#define SERVER_LOGIN_BC         4000 // 玩家进入房间的回应
#define SERVER_LOGIN_FAIL		4002 // 玩家进入房间失败的回应
#define SERVER_LOGOUT_OK_BC     4004 // 退出房间成功
#define CLIENT_TABLE_INFO       1010 // 获取房间信息
#define SERVER_TABLE_INFO_UC	4006 // 获取房间信息回应
#define CLIENT_READY			1003 // 玩家准备
#define SERVER_READY_BC         4007 // 服务器对玩家准备的回应
#define SERVER_READY_FAIL		4008 // 准备失败
#define SERVER_GAME_START_BC    4009 // 开始一局
#define SERVER_GAME_ENDGAME_BC  4030 // 小结算
#define CLIENT_CHANGE_TABLE     1901 // 请求换桌子
#define SERVER_CHANGE_TABLE_FAIL     4901 // 换桌子失败
#define CLIENT_CANCEL_TRUST     1560 //玩家取消托管


// 非通用协议
#define SERVER_GAME_OUTCARD_BC  4024 // 服务器通知玩家出牌
#define CLIENT_GAME_OUTCARD     1025 // 玩家出牌
#define SERVER_GAME_ACTLIST_UC  4028 // 有操作可选
#define CLIENT_GAME_ACTION		1028 // 玩家作出选择
#define SERVER_GAME_PLAYCARD_BC 4026 // 给玩家发牌
#define SERVER_GAME_BANKERCARD_BC	4027 // 庄家挡底牌
#define SERVER_GAME_SOMEONE_OUT_CARD_BC	4023 // 广播出牌信息
#define SERVER_GAME_SOMEONE_EAT_BC   4029 // 更新进牌
#define SERVER_GAME_OUTCARD_OK_BC	4031 // 出牌成功
#define SERVER_GAME_FETCHCARD_BC	4025 // 客户端摸牌
#define SERVER_GAME_SYNC_HANDCARDS_UC  4033 // 同步手牌

#endif
