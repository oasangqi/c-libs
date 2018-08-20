#pragma once

#include "card.h"
#include <stdint.h>
#include <map>
#include <sstream>

const	int		__NEED_HUXI = 15;
const	int		__HAND_NUMCARD = 21;

#define _FLAGS(x)	(1<<(x))

#define HU_PINGHU		0
#define HU_ZIMO			_FLAGS(1)
#define HU_HONGHU		_FLAGS(2)
#define HU_JIAHONG		_FLAGS(3)
#define HU_ZHENDIAN		_FLAGS(4)
#define HU_WUHU			_FLAGS(5)
#define HU_DUIDUI		_FLAGS(6)

const int RATE_HONGHU = 2;		//红胡	2番	13>红牌>=10
const int RATE_JIAHONG = 4;		//夹红	4番	红牌>=13
const int RATE_YIDIANHONG = 3;	//点胡	3番	红牌=1
const int RATE_WUHU = 4;		//乌胡	4番	红牌=0
const int RATE_DUIDUI = 4;		//对对胡4番 没有绞和吃

// 服务器内部表示的吃牌类型
enum EatType
{
	eatShunZi	= 1,	//顺子吃.
	eatHong		,		//2 7 10
	eatSanta	,		//三塔 (绞)
	eatPeng		,		//碰
	eatWei		,		//偎
	eatWei2		,		//臭偎，（没碰再偎）
	eatPao		,		//跑
	eatTi		,		//提
    eatQishouTi ,       //起手提
	eatKan		,		//坎
	eatDui		,		//对
	eatDan		,		//单牌
	eat123		,		//1 2 3 ,特殊顺子.
	eatNone		=0,
};

struct EatCard
{
	EatType		type;		//类型. 参照EatType
	bool		big;		//是否为大字.
	Card		eatCard;	//进牌.
	CARDS_t		cards;		//辅牌.

	EatCard()
		:eatCard(0)
	{
		type = eatNone;
	}

	void	swap(EatCard& rhs)
	{
		type = rhs.type;
		big = rhs.big;
		eatCard = rhs.eatCard;
		cards.swap(rhs.cards);
	}
};

typedef std::vector<EatCard>		EATCARDS_t;
typedef std::vector<CARDS_t>		HANDCARDS_t;

struct EatHuCards
{
    int xi;
    EATCARDS_t huCards;

    EatHuCards():xi(0) { huCards.clear(); }
};
typedef std::vector<EatHuCards>     EATHUCARDS_t;

//吃牌特别处理.
struct _EatBi {
	int			idx;
	EATCARDS_t	cats;
	//第1个为第1手比法,后面的全为第2手比牌法.
	//第1手比牌为哪个吃法,用idx表示在eats中的下标.
};
typedef std::vector<_EatBi>			_EATBI_t;

typedef std::map<int, int8_t>	MAP_CARDS_t; // <牌，张数>

struct EatChiCard
{
	EATCARDS_t		eats;
	_EATBI_t		bi;
};
/////////////////

struct CARDINFO
{
	bool	fetch;
	Card	card;
};

typedef std::vector<CARDINFO>	VEC_OUTINFO_t;

class HandCards
{
public:
	HandCards();
	~HandCards();


	void	clear(void);

	void	addCard(const Card& card);
	void	dumpHandsCards(std::ostringstream &ostst);
	void	addHandCards(const CARDS_t& cards);
	void	addEatCards(const EATCARDS_t& eats);
	void	addOutCards(const VEC_OUTINFO_t& cards);

	//自动提牌.
	void	__firstBlood(CARDS_t& cards);

	CARDS_t	getCards(void) const;
	const EATCARDS_t& getEatCards(void) const;
	HANDCARDS_t	getHandCards(void)const;
	void	delTianHuCard(Card &del);
	const VEC_OUTINFO_t&	getOutCards(void) const;

	bool	findCard(const Card& card);

	bool	checkOut(const Card& card) const;
	bool	outHandCard(const Card& card);
	bool	removeCard(const Card& card, int num = 1);
	bool	addOutCard(const Card& card, bool fetch = true);
	void	removeOutCard(const Card& card);

	bool	passCard(const Card& card);
	bool	passPeng(const Card& card);
	bool	passHu(const Card& card);
	bool	_isPass(const Card& card) const;
	bool	_isPassHu(const Card& card) const;
	bool	_isPassPeng(const Card& card) const;
	void	_clearPassHu(void);

	bool	_isQishouHu(bool self);
	void	_qiShouHu(uint32_t& huFlags, uint32_t& huXi, uint32_t& rate, std::map<int,int>& parm);

	int		_getCount(void) const { return _countCards; }
	int		_getPaoTimes(void) const { return _paoTimes; }

	//是否可吃
	bool	checkChi(const Card& card, EatChiCard& eatChi) ;
	//是否可碰
	bool	checkPeng(const Card& card) const;
	//是否可偎
	bool	checkWei(const Card& card) const;
	//是否可提
	bool	checkTi(const Card& card) const;
	//是否可跑
	bool	checkPao(const Card& card, bool isOut = false) const;
	//是否可胡
	bool	checkHu(const Card& card, bool self = false, bool fetch = false);
	//是否可胡.检测手中的牌是否能够胡牌.
	bool	checkHu(bool self = false);

	//下面的过程必须先调用与之对应的check过程.
	//吃牌.
	bool	EatChi(const Card& card,const EATCARDS_t& eats);
	//碰牌
	bool	EatPeng(const Card& card);
	//偎
	bool	EatWei(const Card& card);
	//臭偎
	bool	EatWei2(const Card& card);
	//跑
	bool	EatPao(const Card& card);
	//提
	bool	EatTi(const Card& card);

	int		_getHuXi(void) const { return _huXi; }
	int		_getHuXi2(void) const { return _huXi2; }

    void    __chooseBestHu();
	bool	_Hu(bool& huqishouhu, EATCARDS_t& eatCards, uint32_t& huFlags, uint32_t& huXi, int cardNum,
		uint32_t& rate, std::map<int,int>& parm, bool qishouhu=false, int tingHu=0);
	void	_HuHandCards(EATCARDS_t& handCards);

	//是否听牌.
	bool	IsTing(const Card& outCard, CARDS_t& huCards);

	int		_getActiveCard(void) const;
	bool	getActiveCards(CARDS_t &cards) const;

	void	DisableHu(void);
	bool	IsDisableHu(void) const;
	void	DisableEat(void);
	bool	IsDisableEat(void) const;
    int     qishouTiCardSize() const;

	bool	checkPaoFlag(void) const;
	void	clearPaoFlag(void);
	void	setPaoFlag(void);
private:
	int		_getCardCount(const Card& card) const;
	void	_optCardCount(const Card& card, int num);

	static int	__getCardCount(int8_t cards[20], const Card& card);
	static void __optCardCount(int8_t cards[20], const Card& card, int num);
	static void __setCardCount(int8_t cards[20], const Card& card, int num);

	static bool __checkChi(int8_t cards[20], const Card& card, EATCARDS_t& eats);

	bool	_checkChi(const Card& card, EATCARDS_t& eats) const;
	bool	_checkBi(const Card& card, EatChiCard& eatChi, int cardNum);
	bool	_checkPaoHu(const Card& card, bool self = false, bool fetch = false);
	bool	_checkHu(const Card& card, bool self = false, bool fetch = false, bool paohu = false);
	bool	__checkHu(int8_t cards[20], int xi, EATCARDS_t& eats);

    bool    __isMonkey() const;
public:
	void		getCardFlags(uint8_t cardFlags[20]) const;
	void		setCardFlags(uint8_t cardFlags[20]);

	uint8_t		getFlags(void) const;
	void		setFlags(uint8_t flags);
public:
	static bool	__isHong(const Card& card);

	static int	_getIndex(const Card& card);
private:
	EATCARDS_t	_eatCards;	//进牌数据.
	EATCARDS_t	_eatHuCards;//上次检测的胡牌数据. 每次判断时都写脏，下次判断时先清空
    EATHUCARDS_t  _eatHuCardList;//上次检测的胡牌数据集合.
	Card		_huCard;
	VEC_OUTINFO_t	_outCards;	//出牌数据
	int8_t		_cards[20]; //手牌，保存对应牌的张数，例如9代表小十,19代表大拾内存换效率,不用容器. 
	uint8_t		_countCards; // 手牌的张数
	uint8_t		_cardFlags[20];	// 每张牌的臭牌flags，按位由低到高为1表示:不能吃、不能胡、不能碰
	uint8_t		_bigCards;	//大字张数
	uint8_t		_smallCards;//小字
	uint8_t		_redCards;	//红字
	uint8_t		_paoTimes;	// 跑、提牌次数
	uint8_t		_huXi;		//进牌区胡息数.自己看到的.
	uint8_t		_huXi2;		//进牌区胡息数.别人看到的.偎牌别人看不到(全看不到).息数不能显示(如果显示了可以知道是偎的大牌还是小牌).
	bool		_self;		// 是否自摸
	int8_t		_flags; // 按位由低到高为1代表：XXX、禁止胡、禁止吃、(起手2提以上,该标记必须被吃、碰、偎消除后才能胡牌)
};
