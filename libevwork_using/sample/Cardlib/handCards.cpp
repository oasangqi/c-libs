#include "handCards.h"

#include <string.h>
#include <iterator>
#include <algorithm>

HandCards::HandCards()
{

}

HandCards::~HandCards()
{

}

void HandCards::clear(void)
{
	memset(_cards, 0, sizeof(_cards));
	memset(_cardFlags, 0, sizeof(_cardFlags));
	_countCards = 0;
	_bigCards = 0;
	_smallCards = 0;
	_redCards = 0;
	_paoTimes = 0;
	_eatCards.clear();
	_outCards.clear();
	_huXi = 0;
	_huXi2 = 0;
	_flags = 0;
}

void HandCards::addCard(const Card& card)
{
	if (!card.IsValid()) return;
	int idx = _getIndex(card);
	_cards[idx]++;
	++_countCards;

	if (card.IsBig()) _bigCards++;
	else _smallCards++;

	int point = card.getPoint();

	if (point == 2 || point == 7 || point == 10)
		_redCards++;
}

void HandCards::dumpHandsCards(std::ostringstream &ostst)
{
	for (int i = 0; i < 20; i++) {
		for (int j = 0; j < _cards[i]; j++) {
			ostst << i + 1 + (i > 9 ? 6 : 0) << ",";
		}
	}
}

void HandCards::addHandCards(const CARDS_t& cards)
{
	clear();
	for (CARDS_t::const_iterator iter = cards.begin();
		iter != cards.end(); ++iter) {
		addCard(*iter);
	}
}

void HandCards::addEatCards(const EATCARDS_t& eats)
{
	_eatCards.clear();
	_huXi = 0;
	_huXi2 = 0;
	_paoTimes = 0;
	std::copy(eats.begin(), eats.end(), std::back_inserter(_eatCards));
	EATCARDS_t::const_iterator iter = _eatCards.begin();
	int xi;
	for (; iter != _eatCards.end(); ++iter) {
		const EatCard& eat = *iter;

		bool isBig = false;
		bool isRed = false;

		if (eat.eatCard.IsBig()) isBig = true;
		if (__isHong(eat.eatCard)) isRed = true;

		int type = (int)iter->type;
		bool  big = iter->big;
		switch (type) {
		case eatShunZi:
			if (isBig) _bigCards += 3;
			else _smallCards += 3;

			if (isRed) {
				_redCards++;
			}
			else {
				if (__isHong(eat.cards[0])) ++_redCards;
				if (__isHong(eat.cards[1])) ++_redCards;
			}
			break;
		case eatSanta:
			if (isBig)++_bigCards;
			else ++_smallCards;
			if (eat.cards[0].IsBig())++_bigCards;
			else ++_smallCards;
			if (eat.cards[1].IsBig())++_bigCards;
			else ++_smallCards;
			if (isRed) ++_redCards;
			if (__isHong(eat.cards[0])) ++_redCards;
			if (__isHong(eat.cards[1])) ++_redCards;
			break;
		case eat123:
			xi = big ? 6 : 3;
			_huXi += xi;
			_huXi2 += xi;

			if (isBig) _bigCards += 3;
			else _smallCards += 3;
			if (isRed) {
				_redCards++;
			}
			else {
				if (__isHong(eat.cards[0])) ++_redCards;
				if (__isHong(eat.cards[1])) ++_redCards;
			}
			break;
		case eatHong:
			xi = big ? 6 : 3;
			_huXi += xi;
			_huXi2 += xi;

			if (isBig) _bigCards += 3;
			else _smallCards += 3;
			_redCards += 3;
			break;
		case eatPeng:
			xi = big ? 3 : 1;
			_huXi += xi;
			_huXi2 += xi;

			if (isBig) _bigCards += 3;
			else _smallCards += 3;
			if (isRed) _redCards += 3;
			break;
		case eatWei:
			xi = big ? 6 : 3;
			_huXi += xi;

			if (isBig) _bigCards += 3;
			else _smallCards += 3;
			if (isRed) _redCards += 3;
			break;
		case eatWei2:
			xi = big ? 6 : 3;
			_huXi += xi;
			_huXi2 += xi;

			if (isBig) _bigCards += 3;
			else _smallCards += 3;
			if (isRed) _redCards += 3;
			break;
		case eatPao:
			xi = big ? 9 : 6;
			_huXi += xi;
			_huXi2 += xi;
			++_paoTimes;

			if (isBig) _bigCards += 4;
			else _smallCards += 4;
			if (isRed) _redCards += 4;
			break;
		case eatTi:
        case eatQishouTi:
			xi = big ? 12 : 9;
			_huXi += xi;
			_huXi2 += xi;
			++_paoTimes;

			if (isBig) _bigCards += 4;
			else _smallCards += 4;
			if (isRed) _redCards += 4;
			break;
		}
	}
}

void HandCards::addOutCards(const VEC_OUTINFO_t& cards)
{
	for (VEC_OUTINFO_t::const_iterator iter = cards.begin(); iter != cards.end(); ++iter) {
		const CARDINFO& info = *iter;
		addOutCard(info.card, info.fetch);
	}
}

// cards保存起手提的牌
void HandCards::__firstBlood(CARDS_t& cards)
{
	cards.clear();
	//发完牌,自动提牌.
	for (int i = 0; i < 20; ++i) {
		int8_t num = _cards[i];
		if (num == 4){
			const Card& card = Card::makeCard(i + 1 - (i >= 10 ? 10 : 0), i >= 10);
			EatCard eat;
			eat.big = card.IsBig();
			eat.eatCard = card;
			eat.type = eatQishouTi;
			_eatCards.push_back(eat);
			removeCard(card, 4);

			int xi = card.IsBig() ? 12 : 9;
			_huXi += xi;
			_huXi2 += xi;
			cards.push_back(card);

			++_paoTimes;
		}
	}
	if (_paoTimes >= 2)
		setPaoFlag(); // 标记起手两提以上
}

CARDS_t HandCards::getCards(void) const
{
	CARDS_t cards;
	for (int i = 0; i < 20; ++i) {
		int8_t num = _cards[i];
		for (; num > 0; --num) {
			cards.push_back(Card::makeCard(i + 1 - (i >= 10 ? 10 : 0), i >= 10));
		}
	}
	return cards;
}

const EATCARDS_t& HandCards::getEatCards(void) const
{
	return _eatCards;
}

void HandCards::delTianHuCard(Card &del)
{
	for (EATCARDS_t::iterator it = _eatCards.begin(); it != _eatCards.end(); ++it) {
		if (it->type == eatQishouTi && it->eatCard == del) {
			_eatCards.erase(it);
			--_paoTimes;
			if (_paoTimes < 2) {
				clearPaoFlag();
			}
			int xi = del.IsBig() ? 12 : 9;
			_huXi -= xi;
			_huXi2 -= xi;
			if (__isHong(del))
				_redCards -= 4;
			if (del.IsBig()) 
				_bigCards -= 4;
			else
				_smallCards -= 4;
			addCard(del);
			addCard(del);
			addCard(del);
			return;
		}
	}
	removeCard(del, 1);
	if (__isHong(del))
		_redCards--;
	if (del.IsBig()) 
		_bigCards--;
	else
		_smallCards--;
	return;
}

static bool comp_cards(const CARDS_t& l, const CARDS_t& r)
{
	CARDS_t::size_type lSize = l.size();
	CARDS_t::size_type rSize = r.size();
	if (lSize <= 0)
		return true;
	if (rSize <= 0)
		return false;
	if (lSize > 1 && rSize > 1)
		return l[1].getVal() < r[1].getVal() ? true : false;
	return l[0].getVal() < r[0].getVal() ? true : false;
}

// 给客户端发送已整理的牌
HANDCARDS_t HandCards::getHandCards(void) const
{
	HANDCARDS_t handCards;
	MAP_CARDS_t	_tmpCards; // 单牌和对牌

	for (int i = 0; i < 20; ++i) {
		int8_t num = _cards[i];
		Card	temp = Card::makeCard(i + 1 - (i >= 10 ? 10 : 0), i >= 10);
		if (num == 1 || num == 2) {
			_tmpCards[temp.getVal()] = num;
		}
		else if (num > 0){
			handCards.push_back(CARDS_t());
			CARDS_t& cards = handCards.back();
			for (int j = 0; j < num; ++j)
				cards.push_back(temp);
		}
	}
	MAP_CARDS_t dui; // < <单牌，1> >
	MAP_CARDS_t dan; // < <对牌，2> >
	for (MAP_CARDS_t::const_iterator iter = _tmpCards.begin();
		iter != _tmpCards.end(); ++iter) {
		int cardNum = iter->second;
		if (cardNum == 0) 
			continue;
		if (cardNum == 1) {
			dan.insert(std::make_pair(iter->first, cardNum));
		}
		else if (cardNum == 2) {
			dui.insert(std::make_pair(iter->first, cardNum));
		}
	}

	//单牌和单牌,或者和对子组成三塔.剩下的对子不处理.
	for (MAP_CARDS_t::iterator iter = dan.begin(); iter != dan.end(); ++iter) {
		int cardNum = iter->second;
		if (cardNum == 0) continue;
		int val = iter->first;
		Card temp(val);
		MAP_CARDS_t::iterator iter1;
		MAP_CARDS_t::iterator iter2;

		// 单牌 大小2 7 10
		int ptVal = temp.getPoint();
		if (ptVal == 2 || ptVal == 7 || ptVal == 10) {
			const Card& cc1 = Card::makeCard(2, temp.IsBig());
			const Card& cc2 = Card::makeCard(7, temp.IsBig());
			const Card& cc3 = Card::makeCard(10, temp.IsBig());

			if (ptVal == 2) {
				iter1 = dan.find(cc2.getVal());
				iter2 = dan.find(cc3.getVal());
				if (iter1 != dan.end() && iter2 != dan.end()) {
					if (iter1->second > 0 && iter2->second > 0) {
						iter1->second--;
						iter2->second--;
						iter->second--;
						handCards.push_back(CARDS_t());
						CARDS_t& cards = handCards.back();
						cards.push_back(temp);
						cards.push_back(cc2);
						cards.push_back(cc3);
						continue;
					}
				}
			}
			else if (ptVal == 7) {
				iter1 = dan.find(cc1.getVal());
				iter2 = dan.find(cc3.getVal());
				if (iter1 != dan.end() && iter2 != dan.end()) {
					if (iter1->second > 0 && iter2->second > 0) {
						iter1->second--;
						iter2->second--;
						iter->second--;
						handCards.push_back(CARDS_t());
						CARDS_t& cards = handCards.back();
						cards.push_back(cc1);
						cards.push_back(temp);
						cards.push_back(cc3);
						continue;
					}
				}
			}
			else {
				iter1 = dan.find(cc1.getVal());
				iter2 = dan.find(cc2.getVal());
				if (iter1 != dan.end() && iter2 != dan.end()) {
					if (iter1->second > 0 && iter2->second > 0) {
						iter1->second--;
						iter2->second--;
						iter->second--;
						handCards.push_back(CARDS_t());
						CARDS_t& cards = handCards.back();
						cards.push_back(cc1);
						cards.push_back(cc2);
						cards.push_back(temp);
						continue;
					}
				}
			}
		}

		//顺子吃.
		//单排的非2710所有吃
		//c1+c2, c1+c3, c3+c4
		int c1 = val + 1;
		int c2 = val + 2;
		int c3 = val - 1;
		int c4 = val - 2;


		// c c+1 c+2
		iter1 = dan.find(c1);
		iter2 = dan.find(c2);
		if (iter1 != dan.end() && iter2 != dan.end()) {
			if (iter1->second > 0 && iter2->second > 0) {
				iter1->second--;
				iter2->second--;
				iter->second--;
				handCards.push_back(CARDS_t());
				CARDS_t& cards = handCards.back();
				cards.push_back(temp);
				cards.push_back(c1);
				cards.push_back(c2);
				continue;
			}
		}
		// c-1 c c+1
		iter1 = dan.find(c1);
		iter2 = dan.find(c3);
		if (iter1 != dan.end() && iter2 != dan.end()) {
			if (iter1->second > 0 && iter2->second > 0) {
				iter1->second--;
				iter2->second--;
				iter->second--;
				handCards.push_back(CARDS_t());
				CARDS_t& cards = handCards.back();
				cards.push_back(c3);
				cards.push_back(temp);
				cards.push_back(c1);
				continue;
			}
		}

		// c c+1 c+2
		iter1 = dan.find(c3);
		iter2 = dan.find(c4);
		if (iter1 != dan.end() && iter2 != dan.end()) {
			if (iter1->second > 0 && iter2->second > 0) {
				iter1->second--;
				iter2->second--;
				iter->second--;
				handCards.push_back(CARDS_t());
				CARDS_t& cards = handCards.back();
				cards.push_back(c4);
				cards.push_back(c3);
				cards.push_back(temp);
				continue;
			}
		}

		// 三塔
		if (temp.IsBig())
			iter1 = dui.find(Card::makeCard(ptVal, false).getVal());
		else 
			iter1 = dui.find(Card::makeCard(ptVal, true).getVal());

		if (iter1 != dui.end()) {
			iter->second--;
			handCards.push_back(CARDS_t());
			CARDS_t& cards = handCards.back();
			cards.push_back(temp);
			cards.push_back(iter1->first);
			cards.push_back(iter1->first);
			dui.erase(iter1);
			continue;
		}
	}

	// 不能组合2 7 10、顺子、三塔的单牌
	for (MAP_CARDS_t::iterator iter = dan.begin(); iter != dan.end(); ++iter) {
		int cardNum = iter->second;
		if (cardNum == 0) continue;
		int val = iter->first;
		Card temp(val);
		int ptVal = temp.getPoint();
		handCards.push_back(CARDS_t());

		CARDS_t& cards = handCards.back();
		cards.push_back(temp);
		iter->second--;

		bool isBig = temp.IsBig();

		// 2 7、2 10、7 10
		if (ptVal == 2 || ptVal == 7 || ptVal == 10) {
			Card card1 = Card::makeCard(2, isBig);
			MAP_CARDS_t::iterator it = dan.find(card1.getVal());
			if (it != dan.end() && it != iter && it->second > 0) {
				it->second--;
				cards.push_back(card1);
			}
			card1 = Card::makeCard(7, isBig);
			it = dan.find(card1.getVal());
			if (it != dan.end() && it != iter && it->second > 0) {
				it->second--;
				cards.push_back(card1);
			}
			card1 = Card::makeCard(10, isBig);
			it = dan.find(card1.getVal());
			if (it != dan.end() && it != iter && it->second > 0) {
				it->second--;
				cards.push_back(card1);
			}
		}
		else {
			do
			{
				// c-2 c、c-1 c、 c c+1、 c c+2
				Card card1 = temp - 1;
				if (card1.IsValid()) {
					MAP_CARDS_t::iterator it = dan.find(card1.getVal());
					if (it != dan.end() && it->second > 0) {
						it->second--;
						cards.push_back(card1);
						break;
					}
				}
				card1 = temp + 1;
				if (card1.IsValid()) {
					MAP_CARDS_t::iterator it = dan.find(card1.getVal());
					if (it != dan.end() && it->second > 0) {
						it->second--;
						cards.push_back(card1);
						break;
					}
				}
				card1 = temp - 2;
				if (card1.IsValid()) {
					MAP_CARDS_t::iterator it = dan.find(card1.getVal());
					if (it != dan.end() && it->second > 0) {
						it->second--;
						cards.push_back(card1);
						break;
					}
				}
				card1 = temp + 2;
				if (card1.IsValid()) {
					MAP_CARDS_t::iterator it = dan.find(card1.getVal());
					if (it != dan.end() && it->second > 0) {
						it->second--;
						cards.push_back(card1);
						break;
					}
				}
			} while (false);
		}
		if (cards.size() > 1) std::sort(cards.begin(), cards.end(), std::less<Card>());
	}

	// 对子单独发
	for (MAP_CARDS_t::iterator iter = dui.begin(); iter != dui.end(); ++iter) {
		Card temp(iter->first);
		handCards.push_back(CARDS_t());
		CARDS_t& cards = handCards.back();
		cards.push_back(temp);
		cards.push_back(temp);
	}

	std::sort(handCards.begin(), handCards.end(), comp_cards);

	return handCards;
}

const VEC_OUTINFO_t& HandCards::getOutCards(void) const
{
	return _outCards;
}

bool HandCards::findCard(const Card& card)
{
	int idx = _getIndex(card);
	return _cards[idx] > 0;
}

bool HandCards::checkOut(const Card& card) const
{
	if (!card.IsValid())
		return false;
	int idx = _getIndex(card);
	if (_cards[idx] > 0 && _cards[idx] < 3) {
		return true;
	}
	return false;
}

bool HandCards::outHandCard(const Card& card)
{
	if (!checkOut(card))
		return false;
	if (!removeCard(card, 1))
		return false;
	if (card.IsBig())
		--_bigCards;
	else
		--_smallCards;
	if (__isHong(card))
		--_redCards;
	passCard(card);
	return true;
}

bool HandCards::removeCard(const Card& card, int num /*= 1*/)
{
	if (!card.IsValid())
		return false;
	int idx = _getIndex(card);
	if (_cards[idx] >= num) {
		_cards[idx] -= num;
		_countCards -= num;
		return true;
	}
	return false;
}

bool HandCards::addOutCard(const Card& card, bool fetch)
{
	if (!card.IsValid()) return true;
	//自己打出去的不能吃
	passCard(card);
	CARDINFO info;
	info.card = card;
	info.fetch = fetch;
	_outCards.push_back(info);
	return false;
}

void HandCards::removeOutCard(const Card& card)
{
	int si = (int)_outCards.size();
	for (int i = --si; i >= 0; --i) {
		if (_outCards[i].card == card) {
			_outCards.erase(_outCards.begin() + i);
			return;
		}
	}
}

// 应该为passEat
bool HandCards::passCard(const Card& card)
{
	if (!card.IsValid()) return true;
	int idx = _getIndex(card);
	_cardFlags[idx] |= 1;
	return true;
}

bool HandCards::passPeng(const Card& card)
{
	if (!card.IsValid()) return true;
	int idx = _getIndex(card);
	_cardFlags[idx] |= 4;
	return true;
}

bool HandCards::passHu(const Card& card)
{
    return true;
	if (!card.IsValid()) return true;
	int idx = _getIndex(card);
	_cardFlags[idx] |= 2;
	return true;
}

bool HandCards::_isPass(const Card& card) const
{
	if (!card.IsValid()) return true;
	int idx = _getIndex(card);
	return (_cardFlags[idx] & 1) ? true : false;
}

bool HandCards::_isPassHu(const Card& card) const
{
    return false;
	if (!card.IsValid()) return true;
	int idx = _getIndex(card);
	return (_cardFlags[idx] & 2) ? true : false;
}

bool HandCards::_isPassPeng(const Card& card) const
{
	if (!card.IsValid()) return true;
	int idx = _getIndex(card);
	return (_cardFlags[idx] & 4) ? true : false;
}

void HandCards::_clearPassHu(void)
{
	for (int i = 0; i < 20; ++i) {
		_cardFlags[i] &= ~2;
	}
}

/*起手特殊胡检测*/
bool HandCards::_isQishouHu(bool self) 
{
	int ti = 0;
	int kan = 0;
	for (int i = 0; i < 20; ++i) {
		if (_cards[i] == 3) {
			++kan;
		}
	}
    for (EATCARDS_t::const_iterator cit = _eatCards.begin(); cit != _eatCards.end(); ++cit) {
        if (cit->type == eatQishouTi) {
            ++ti;
        }
    }

	if (ti >= 3 || kan >= 5 || (ti == 1 && kan == 4) || (ti == 2 && kan == 3))
    {
        _self = self;
        return true;
    }
    return false;
}

/*5坎3提胡,最大化胡息*/
void HandCards::_qiShouHu(uint32_t& huFlags, uint32_t& huXi, uint32_t& rate, std::map<int,int>& parm)
{
	//_eatCards.clear();
	_eatHuCards.clear();

	huFlags = HU_PINGHU;
	rate = 0;
	huXi = 0;
	parm.clear();

	for (int i = 0; i < 20; ++i) {
		Card	temp = Card::makeCard(i + 1 - (i >= 10 ? 10 : 0), i >= 10);

		if (_cards[i] == 3) {
			EatCard eat;
			eat.type = eatKan;
			eat.eatCard = temp;
			eat.big = temp.IsBig();
			_eatHuCards.push_back(eat);
			_cards[i] = 0;
			huXi += temp.IsBig() ? 6 : 3;
		}
		else if (_cards[i] == 4) {
			EatCard eat;
			eat.type = eatTi;
			eat.eatCard = temp;
			eat.big = temp.IsBig();
			_eatHuCards.push_back(eat);
			_cards[i] = 0;
			huXi += temp.IsBig() ? 12 : 9;
		}
	}

    for (EATCARDS_t::const_iterator cit = _eatCards.begin(); cit != _eatCards.end(); ++cit) {
        // 只会有提的情况
        huXi += cit->eatCard.IsBig() ? 12 : 9;
    }

	//二七十/贰柒拾、一二三/壹贰叁.

	int arrEats[] = {
		0x02,0x07,0x0A,
		0x01,0x02,0x03,
		0x12,0x17,0x1A,
		0x11,0x12,0x13,
	};

	for (unsigned int i = 0; i < sizeof(arrEats) / sizeof(arrEats[0]); i += 3) {
		Card c1(arrEats[i]);
		Card c2(arrEats[i + 1]);
		Card c3(arrEats[i + 2]);
		int n1 = _getCardCount(c1);
		int n2 = _getCardCount(c2);
		int n3 = _getCardCount(c3);
		while (n1 > 0 && n2 > 0 && n3 > 0) {
			_optCardCount(c1, -1);
			_optCardCount(c2, -1);
			_optCardCount(c3, -1);
			--n1;
			--n2;
			--n3;
			EatCard eat;
			eat.type = c1.getPoint() == 1 ? eat123 : eatHong;
			eat.big = c1.IsBig();
			eat.eatCard = c1;
			eat.cards.push_back(c2);
			eat.cards.push_back(c3);
			_eatHuCards.push_back(eat);

			huXi += c1.IsBig() ? 6 : 3;
		}
	}

	CARDS_t dan;
	for (int i = 0; i < 20; ++i) {
		if (_cards[i] <= 0) continue;
		Card temp = Card::makeCard(i + 1 - (i >= 10 ? 10 : 0), i >= 10);
		if (_cards[i] == 2) {
			EatCard eat;
			eat.type = eatDui;
			eat.eatCard = temp;
			_eatHuCards.push_back(eat);
		}
		else {
			dan.push_back(temp);
			if (dan.size() >= 4) {
				// 四张单牌组合
				EatCard eat;
				eat.type = eatDan;
				eat.cards.swap(dan);
				_eatHuCards.push_back(eat);
			}
		}
	}
	if (dan.size()) {
		// 多余的单牌组合
		EatCard eat;
		eat.type = eatDan;
		eat.cards.swap(dan);
		_eatHuCards.push_back(eat);
	}


	//全名堂
	// 庄家起手天胡，只算自摸
	if (_self) {
        huFlags |= HU_ZIMO;
        //huXi += 3;
	}

	// 名堂
	if (_redCards >= 10 && _redCards < 13) {
		huFlags |= HU_HONGHU;
		rate += RATE_HONGHU;
        //parm[HU_HONGHU] = _redCards;
	}
	else if (_redCards >= 13) {
		huFlags |= HU_JIAHONG;
		rate += RATE_JIAHONG;
        //parm[HU_JIAHONG] = _redCards;
	}
	else if (_redCards == 1) {
		huFlags |= HU_ZHENDIAN;
		rate += RATE_YIDIANHONG;
	}
	else if (_redCards == 0) {
		huFlags |= HU_WUHU;
		rate += RATE_WUHU;
	}

	bool isDuizi = true;
    bool isHanghangxi = true;
	for (EATCARDS_t::const_iterator it = _eatHuCards.begin(); it != _eatHuCards.end(); ++it) {
		if (it->type != eatWei && it->type != eatKan && 
            it->type != eatWei2 && it->type != eatPeng && 
            it->type != eatPao && it->type != eatTi 
            && it->type != eatQishouTi)
		{
            if (it->type != eatDui)
			    isDuizi = false;
            if (it->type != eatHong && it->type != eat123)
                isHanghangxi = false;
		}
	}
	if (isDuizi) {
		huFlags |= HU_DUIDUI;
		rate += RATE_DUIDUI;
	}

    if (rate <= 1) ++rate;
}

bool HandCards::checkChi(const Card& card, EatChiCard& eatChi)
{
	if (!card.IsValid()) return false;
	if (IsDisableEat()) return false;
	//臭牌检测.
	if (_isPass(card)) return false;
	int cardNum = _getCardCount(card);
	if (cardNum >= 3) return false;
	if (_getActiveCard() <= 2) return false;

	eatChi.eats.clear();
	eatChi.bi.clear();
	bool result = _checkChi(card, eatChi.eats);
	if (cardNum == 0) {
		return result;
	}
	result = _checkBi(card, eatChi, cardNum);
	return result;
}

bool HandCards::checkPeng(const Card& card) const
{
	if (!card.IsValid()) return false;
	if (IsDisableEat()) return false;
	int idx = _getIndex(card);
	//臭牌检测.
	if (_isPassPeng(card)) return false;
	if (_isPass(card)) return false;
	if (_getActiveCard() <= 2) return false;
	return _cards[idx] == 2;
}

bool HandCards::checkWei(const Card& card) const
{
	if (!card.IsValid()) return false;
	int idx = _getIndex(card);
	return _cards[idx] == 2;
}

// 只有自己摸的牌才能提
bool HandCards::checkTi(const Card& card) const
{
	if (!card.IsValid()) return false;

	// 可用坎牌提
	int idx = _getIndex(card);
	if (_cards[idx] == 3) return true;

	// 可用偎牌提
	for (EATCARDS_t::const_iterator iter = _eatCards.begin();
		iter != _eatCards.end(); ++iter) {
		if (iter->eatCard == card) {
			if (iter->type == eatWei) {
				return true;
			}
			break;
		}
	}
	return false;
}

// 参数isOut表示是否是从手里打出的牌
bool HandCards::checkPao(const Card& card, bool isOut /*= false*/) const
{
	if (!card.IsValid()) return false;
	// 可用坎牌偎
	int idx = _getIndex(card);
	if (_cards[idx] == 3) return true;

	// 可用进牌偎
	for (EATCARDS_t::const_iterator iter = _eatCards.begin();
		iter != _eatCards.end(); ++iter) {
		if (iter->eatCard == card) {
			if (iter->type == eatWei) {
				return true;
			}else if(iter->type==eatPeng && !isOut){
				return true;
			}
			break;
		}
	}
	return false;
}

// 参数:
// 	self: 是否来自自己
// 	fetch: 是否摸出
// 	判断进张card后能不能胡
bool HandCards::checkHu(const Card& card, bool self, bool fetch)
{
	// 是否死手
	if (IsDisableHu()) {
		return false;
	}

	if (checkPaoFlag()) {
		return false;
	}

	if (_isPassHu(card)) {
        return false;
	}

    _self = self;
    _eatHuCardList.clear();
    _checkPaoHu(card, self, fetch);
    _checkHu(card, self, fetch);
    return !_eatHuCardList.empty();
}

// self表示是否自摸的
// 庄家摸亮张牌、提、跑、偎后判断能不能胡
// 即判断无需再进张时能不能胡
bool HandCards::checkHu(bool self)
{
	if (IsDisableHu()) return false;

    if (checkPaoFlag()) return false;

    _self = self;
    _eatHuCardList.clear();
    return _checkHu(Card(), self);
}

bool HandCards::EatChi(const Card& card,const EATCARDS_t& eats)
{
	if (eats.size() == 0) return false;

	EATCARDS_t::const_iterator iter = eats.begin();
	int c1 = _getCardCount(iter->cards[0]);
	int c2 = _getCardCount(iter->cards[1]);
	if (c1 <= 0 || c2 <= 0 || c1 > 2 || c2 > 2)
		return false;

	int8_t	bakCard[20];
	uint8_t bakHuXi1, bakHuXi2;
	bakHuXi1 = _huXi;
	bakHuXi2 = _huXi2;
	memcpy(bakCard, _cards, sizeof(bakCard));
	EATCARDS_t bakEats = _eatCards;
	int countCardsBak = _countCards;

	bool clearHu = false;

	removeCard(iter->cards[0]);
	removeCard(iter->cards[1]);
	_eatCards.push_back(*iter);
	if (iter->type == eat123 || iter->type == eatHong) {
		int xi = iter->big ? 6 : 3;
		_huXi += xi;
		_huXi2 += xi;
		clearHu = true;
	}
	++iter;
	for (; iter != eats.end(); ++iter) {
		removeCard(iter->eatCard);
		removeCard(iter->cards[0]);
		removeCard(iter->cards[1]);
		_eatCards.push_back(*iter);
		if (iter->type == eat123 || iter->type == eatHong) {
			int xi = iter->big ? 6 : 3;
			_huXi += xi;
			_huXi2 += xi;
			clearHu = true;
		}
	}
	if (_getCardCount(card) > 0) {
		_eatCards = bakEats;
		_huXi = bakHuXi1;
		_huXi2 = bakHuXi2;
		memcpy(_cards, bakCard, sizeof(bakCard));
		_countCards = countCardsBak ;
		return false;
	}
	if (card.IsBig()) ++_bigCards;
	else ++_smallCards;
	if (__isHong(card))
		++_redCards;
	if (clearHu) _clearPassHu();
	return true;
}

bool HandCards::EatPeng(const Card & card)
{
	if (_getCardCount(card) != 2)
		return false;
	if (_getActiveCard() <= 2)
		return false;

	removeCard(card, 2);
	if (__isHong(card))
		++_redCards;
	if (card.IsBig()) ++_bigCards;
	else ++_smallCards;
	int xi = card.IsBig() ? 3 : 1;
	_huXi += xi;
	_huXi2 += xi;

	_eatCards.push_back(EatCard());
	EatCard& eat = _eatCards.back();
	eat.big = card.IsBig();
	eat.eatCard = card;
	eat.type = eatPeng;

	_clearPassHu();
	return true;
}

bool HandCards::EatWei(const Card& card)
{
	if (_getCardCount(card) != 2)
		return false;
	removeCard(card, 2);
	if (__isHong(card))
		++_redCards;
	if (card.IsBig()) ++_bigCards;
	else ++_smallCards;
	int xi = card.IsBig() ? 6 : 3;
	_huXi += xi;
	_eatCards.push_back(EatCard());
	EatCard& eat = _eatCards.back();
	eat.big = card.IsBig();
	eat.eatCard = card;
	eat.type = eatWei;

	_clearPassHu();
	return true;
}

bool HandCards::EatWei2(const Card& card)
{
	if (_getCardCount(card) != 2)
		return false;
	removeCard(card, 2);
	if (__isHong(card))
		++_redCards;
	if (card.IsBig()) ++_bigCards;
	else ++_smallCards;
	int xi = card.IsBig() ? 6 : 3;
	_huXi += xi;
	_huXi2 += xi;

	_eatCards.push_back(EatCard());
	EatCard& eat = _eatCards.back();
	eat.big = card.IsBig();
	eat.eatCard = card;
	eat.type = eatWei2;

	_clearPassHu();
	return true;
}

bool HandCards::EatPao(const Card& card)
{
	if (_getCardCount(card) == 3) {
		removeCard(card, 3);
		if (__isHong(card))
			++_redCards;
		if (card.IsBig()) ++_bigCards;
		else ++_smallCards;
		int xi = card.IsBig() ? 9 : 6;
		_huXi += xi;
		_huXi2 += xi;

		_eatCards.push_back(EatCard());
		EatCard& eat = _eatCards.back();
		eat.big = card.IsBig();
		eat.eatCard = card;
		eat.type = eatPao;
		++_paoTimes;

		_clearPassHu();
		return true;
	}

	EATCARDS_t::iterator iter;
	for (iter = _eatCards.begin(); iter != _eatCards.end(); ++iter) {
		if (iter->eatCard == card){
			if (__isHong(card))
				++_redCards;
			if (card.IsBig()) ++_bigCards;
			else ++_smallCards;
			if (iter->type == eatPeng) {
				iter->type = eatPao;
				int xi = card.IsBig() ? 6 : 5;//碰,大3小1
				_huXi += xi;
				_huXi2 += xi;
				++_paoTimes;
				// TOASK
				if (iter + 1 != _eatCards.end()) {
					EatCard eat;
					eat.swap(*iter);
					_eatCards.erase(iter);
					_eatCards.push_back(eat);
				}
				_clearPassHu();
				return true;
			}
			else if (iter->type == eatWei) {
				iter->type = eatPao;
				int xi = 3;//偎,大6小3.
				_huXi += xi;
				_huXi2 += xi;
				++_paoTimes;
				if (iter + 1 != _eatCards.end()) {
					EatCard eat;
					eat.swap(*iter);
					_eatCards.erase(iter);
					_eatCards.push_back(eat);
				}
				_clearPassHu();
				return true;
			}
			if (__isHong(card))
				--_redCards;
			if (card.IsBig()) --_bigCards;
			else --_smallCards;
			return	false;
		}
	}
	return false;
}

// 提牌
bool HandCards::EatTi(const Card& card)
{
	// 用坎牌提
	if (_getCardCount(card) == 3) {
		removeCard(card, 3);
		if (__isHong(card))
			++_redCards;
		if (card.IsBig()) ++_bigCards;
		else ++_smallCards;
		int xi = card.IsBig() ? 12 : 9;
		_huXi += xi;
		_huXi2 += xi;

		_eatCards.push_back(EatCard());
		EatCard& eat = _eatCards.back();
		eat.big = card.IsBig();
		eat.eatCard = card;
		eat.type = eatTi;
		++_paoTimes;

		_clearPassHu();
		return true;
	}
	// 用偎牌提
	EATCARDS_t::iterator iter;
	for (iter = _eatCards.begin(); iter != _eatCards.end(); ++iter) {
		if (iter->eatCard == card) {
			if (__isHong(card))
				++_redCards;
			if (card.IsBig()) ++_bigCards;
			else ++_smallCards;
			if (iter->type == eatWei) {
				iter->type = eatTi;
				int xi = 6;//偎,大6小3.
				_huXi += xi;
				_huXi2 += xi;
				++_paoTimes;
				// 去掉偎，新增的提放到最后
				if (iter + 1 != _eatCards.end()) {
					EatCard eat;
					eat.swap(*iter);
					_eatCards.erase(iter);
					_eatCards.push_back(eat);
				}
				_clearPassHu();
				return true;
			}
			if (__isHong(card))
				--_redCards;
			if (card.IsBig()) --_bigCards;
			else --_smallCards;
			return	false;
		}
	}
	return false;
}

//根据需求定制不同最优策略
void HandCards::__chooseBestHu()
{
    int maxXi = 0;
    int maxIndex = 0;
    int index = 0;
    for (EATHUCARDS_t::const_iterator cit = _eatHuCardList.begin(); cit != _eatHuCardList.end(); ++cit) {
        if (maxXi < cit->xi) {
            maxXi = cit->xi;
            maxIndex = index;
        }
        ++index;
    }

    _eatHuCards = _eatHuCardList[maxIndex].huCards;
}

bool HandCards::_Hu(bool& huqishouhu, EATCARDS_t& eatCards, uint32_t& huFlags, uint32_t& huXi, int cardNum, 
	uint32_t& rate, std::map<int,int>& parm, bool qishouhu, int tingHu )
{
	/*
	if (_eatHuCards.size() == 0)
		return false;
	*/

	huFlags = HU_PINGHU;

	eatCards.clear();

    if (qishouhu && _isQishouHu(_self))
    {
        _qiShouHu(huFlags, huXi, rate, parm);
		huqishouhu = true;
        return true;
    }
    __chooseBestHu();

	EatCard eat;
	if (_huCard.IsValid()) {
		bool result = false;
		for (EATCARDS_t::iterator it = _eatHuCards.begin(); it != _eatHuCards.end(); ++it) {
			if (it->eatCard == _huCard && it->type != eatKan) {
				eat.swap(*it);
                _eatHuCards.erase(it);
                result = true;
                break;
            }
            else if (it->cards.size() > 0) {
                if (it->cards[0] == _huCard) {
                    eat.swap(*it);
                    eat.cards[0] = eat.eatCard;
                    eat.eatCard = _huCard;
                    _eatHuCards.erase(it);
                    result = true;
                    break;
                }
                else if (it->cards[1] == _huCard) {
                    eat.swap(*it);
                    eat.cards[1] = eat.eatCard;
                    eat.eatCard = _huCard;
                    _eatHuCards.erase(it);
                    result = true;
                    break;
                }
            }
		}

		//跑牌后胡的特殊处理.
		if (!result && EatPao(_huCard)) {
			_huCard.Reset();
			eat.swap(_eatCards.back());
			_eatCards.erase(_eatCards.end() - 1);
		}
	}
	else if (_eatCards.size() > 0) {
		eat.swap(_eatCards.back());
		_eatCards.erase(_eatCards.end() - 1);
	}

	std::copy(_eatCards.begin(), _eatCards.end(), std::back_inserter(eatCards));
	std::copy(_eatHuCards.begin(), _eatHuCards.end(), std::back_inserter(eatCards));
	if (eat.type != eatNone) {
		eatCards.push_back(eat);
		_eatHuCards.push_back(eat);
	}
	int xi = 0;
//	Card birdCard(birdVal);
//	birdVal = 0;

	for (EATCARDS_t::const_iterator it = eatCards.begin(); it != eatCards.end(); ++it) {
		const EatCard& eat = *it;
		bool big = eat.big;
		switch ((int)eat.type) {
			/*
		case eatShunZi:
		case eatSanta:
			if (it->eatCard == birdCard)
				++birdVal;
			if (it->cards[0] == birdCard)
				++birdVal;
			if (it->cards[1] == birdCard)
				++birdVal;
			break;
			*/
		case eat123:
		case eatHong:
			/*
			if (it->eatCard == birdCard)
				++birdVal;
			else if (it->cards[0] == birdCard)
				++birdVal;
			else if (it->cards[1] == birdCard)
				++birdVal;
			*/
			xi += big ? 6 : 3;
			break;
		case eatWei:
		case eatWei2:
		case eatKan:
			xi += big ? 6 : 3;
			/*
			if (it->eatCard == birdCard)
				birdVal += 3;
			*/
			break;
		case eatPao:
			xi += big ? 9 : 6;
			/*
			if (it->eatCard == birdCard)
				birdVal += 4;
			*/
			break;
		case eatTi:
        case eatQishouTi:
			xi += big ? 12 : 9;
			/*
			if (it->eatCard == birdCard)
				birdVal += 4;
			*/
			break;
		case eatPeng:
			xi += big ? 3 : 1;
			/*
			if (it->eatCard == birdCard)
				birdVal += 3;
			*/
			break;
			/*
		case eatDui:
			if (it->eatCard == birdCard)
				birdVal += 2;
			break;
			*/
		}
	}
	if (xi < __NEED_HUXI) return false;
	huXi = xi;
	if (_huCard.IsValid()) {

		if (_huCard.IsBig()) 
			++_bigCards;
		else ++_smallCards;

		if (__isHong(_huCard))
			++_redCards;
	}

	rate = 0;
	parm.clear();

	// 名堂
	if (_redCards >= 10 && _redCards < 13) {
		huFlags |= HU_HONGHU;
		rate += RATE_HONGHU;
       // parm[HU_HONGHU] = _redCards;
	}
	else if (_redCards >= 13) {
		huFlags |= HU_JIAHONG;
		rate += RATE_JIAHONG;
        //parm[HU_JIAHONG] = _redCards;
	}
	else if (_redCards == 1) {
		huFlags |= HU_ZHENDIAN;
		rate += RATE_YIDIANHONG;
	}
	else if (_redCards == 0) {
		huFlags |= HU_WUHU;
		rate += RATE_WUHU;
	}

#if 0
	if (_bigCards >= 18) {
		huFlags |= HU_DA;
		rate += RATE_DA;
		rate += _bigCards - 18;
		parm[HU_DA] = _bigCards;
	}
	else if (_smallCards >= 16) {
		huFlags |= HU_XIAO;
		rate += RATE_XIAO;
		rate += _smallCards - 16;
		parm[HU_XIAO] = _smallCards;
	}
#endif

	bool isDuizi = true;
    bool isHanghangxi = true;
	for (EATCARDS_t::const_iterator it = eatCards.begin(); it != eatCards.end(); ++it) {
		if (it->type != eatWei && it->type != eatKan && 
            it->type != eatWei2 && it->type != eatPeng && 
            it->type != eatPao && it->type != eatTi && it->type != eatQishouTi)
		{
            if (it->type != eatDui)
			    isDuizi = false;
            if (it->type != eatHong && it->type != eat123)
                isHanghangxi = false;
		}
	}
	if (isDuizi) {
		huFlags |= HU_DUIDUI;
		rate += RATE_DUIDUI;
	}
	// 自摸加3息，前提是之前已经够15息
	if (_self) {
		huFlags |= HU_ZIMO;
		//huXi += 3;
	}

	// 没有翻番，则为1番
	if (rate <= 1) 
		++rate;

	return true;
}

void HandCards::_HuHandCards(EATCARDS_t& handCards)
{
	handCards.clear();
	std::copy(_eatHuCards.begin(), _eatHuCards.end(), std::back_inserter(handCards));
}

bool HandCards::IsTing(const Card& outCard, CARDS_t& huCards)
{
	bool paoFlag = checkPaoFlag();
	if (paoFlag) clearPaoFlag();

	huCards.clear();
	_optCardCount(outCard, -1);
	//暴力测试每一张是否能胡..
	for (int i = 0; i < 20; ++i) {
		const Card& card = Card::makeCard(i + 1 - (i >= 10 ? 10 : 0), i >= 10);
		if (checkHu(card,true)) huCards.push_back(card);
	}
	_optCardCount(outCard, 1);

	if (paoFlag) 
		setPaoFlag();
	return huCards.size() > 0;
}

int HandCards::_getActiveCard(void) const
{
	int numActive = 0;
	for (int i = 0; i < 20; ++i) {
		int8_t num = _cards[i];
		if (num > 0 && num < 3) {
			numActive += num;
		}
	}
	return numActive;
}

bool HandCards::getActiveCards(CARDS_t &cards) const
{
	int cdVal = 0;
	cards.clear();

	for (int i = 0; i < 20; ++i) {
		int8_t num = _cards[i];
		if (num > 0 && num < 3) {
			cdVal = i + 1 - (i >= 10 ? 10 : 0);
			cards.push_back(Card::makeCard(cdVal, i >= 10));
		}
	}
	return cards.size() > 0;
}

void HandCards::DisableHu(void)
{
	_flags |= (1 << 1);
}

// 是否死手
bool HandCards::IsDisableHu(void) const
{
	return (_flags&(1 << 1)) ? true : false;
}

void HandCards::DisableEat(void)
{
	_flags |= (1 << 2);
}

bool HandCards::IsDisableEat(void) const
{
	return (_flags&(1 << 2)) ? true : false;
}

bool HandCards::checkPaoFlag(void) const
{
	return (_flags&(1 << 3)) ? true : false;
}

void HandCards::clearPaoFlag(void)
{
	_flags &= ~(1 << 3);
}

void HandCards::setPaoFlag(void)
{
	_flags |= (1 << 3);
}

int HandCards::_getCardCount(const Card& card) const
{
	if (!card.IsValid()) return 0;
	int idx = _getIndex(card);
	return _cards[idx];
}

void HandCards::_optCardCount(const Card& card, int num)
{
	if (!card.IsValid()) return;
	int idx = _getIndex(card);
	_cards[idx] += num;
}

int HandCards::__getCardCount(int8_t cards[20], const Card& card)
{
	if (!card.IsValid()) return 0;
	int idx = _getIndex(card);
	return cards[idx];
}

void HandCards::__optCardCount(int8_t cards[20], const Card& card, int num)
{
	if (!card.IsValid()) return;
	int idx = _getIndex(card);
	cards[idx] += num;
}

void HandCards::__setCardCount(int8_t cards[20], const Card& card, int num)
{
	if (!card.IsValid()) return;
	int idx = _getIndex(card);
	cards[idx] = num;
}

// 参数：
// 	cards：手牌
// 	card：待吃的牌
// 	eats：吃下的牌
// 注：优先按息多的方式吃牌
bool HandCards::__checkChi(int8_t cards[20], const Card& card, EATCARDS_t& eats)
{
	eats.clear();
	int point = card.getPoint();
	bool isBig = card.IsBig();

	//无效牌.数量为0,比如 判断是否能吃 小1,point 为 1, -2 为无效牌.参考Card构造函数.
	//顺子判断
	Card temp1;
	Card temp2;
	int c1, c2;

	//2 7 10 判断
	if (point == 2) {
		temp1 = Card::makeCard(7, isBig);
		temp2 = Card::makeCard(10, isBig);
		c1 = __getCardCount(cards, temp1);
		c2 = __getCardCount(cards, temp2);
		if (c1 > 0 && c2 > 0/* && c1 < 3 && c2 < 3*/) {
			EatCard eat;
			eat.type = eatHong;
			eat.big = isBig;
			eat.eatCard = card;
			eat.cards.push_back(temp1);
			eat.cards.push_back(temp2);
			eats.push_back(eat);
		}
	}
	else if (point == 7) {
		temp1 = Card::makeCard(2, isBig);
		temp2 = Card::makeCard(10, isBig);
		c1 = __getCardCount(cards, temp1);
		c2 = __getCardCount(cards, temp2);
		if (c1 > 0 && c2 > 0/* && c1 < 3 && c2 < 3*/) {
			EatCard eat;
			eat.type = eatHong;
			eat.big = isBig;
			eat.eatCard = card;
			eat.cards.push_back(temp1);
			eat.cards.push_back(temp2);
			eats.push_back(eat);
		}
	}
	else if (point == 10) {
		temp1 = Card::makeCard(2, isBig);
		temp2 = Card::makeCard(7, isBig);
		c1 = __getCardCount(cards, temp1);
		c2 = __getCardCount(cards, temp2);
		if (c1 > 0 && c2 > 0/* && c1 < 3 && c2 < 3*/) {
			EatCard eat;
			eat.type = eatHong;
			eat.big = isBig;
			eat.eatCard = card;
			eat.cards.push_back(temp1);
			eat.cards.push_back(temp2);
			eats.push_back(eat);
		}
	}

    temp1 = Card::makeCard(point - 1, isBig);
    temp2 = Card::makeCard(point - 2, isBig);
    c1 = __getCardCount(cards,temp1);
    c2 = __getCardCount(cards,temp2);
    if (c1 > 0 && c2 > 0 && c1 < 3 && c2 < 3) {
        EatCard eat;
        eat.type = point == 3 ? eat123 : eatShunZi;
        eat.big = isBig;
        eat.eatCard = card;
        eat.cards.push_back(temp1);
        eat.cards.push_back(temp2);
        eats.push_back(eat);
    }

    temp2 = Card::makeCard(point + 1, isBig);
    c2 = __getCardCount(cards,temp2);
	if (c1 > 0 && c2 > 0/* && c1 < 3 && c2 < 3*/) {
		EatCard eat;
		eat.type = point == 2 ? eat123 : eatShunZi;
		eat.big = isBig;
		eat.eatCard = card;
		eat.cards.push_back(temp2);
		eat.cards.push_back(temp1);
		eats.push_back(eat);
	}

	temp1 = Card::makeCard(point + 2, isBig);
	c1 = __getCardCount(cards,temp1);
	if (c2 > 0 && c1 > 0/* && c2 < 3 && c1 < 3*/) {
		EatCard eat;
		eat.type = point == 1 ? eat123 : eatShunZi;
		eat.big = isBig;
		eat.eatCard = card;
		eat.cards.push_back(temp1);
		eat.cards.push_back(temp2);
        eats.push_back(eat);
    }

    //三塔判断
    temp1 = Card::makeCard(point, false);
    c1 = __getCardCount(cards,temp1);
    //小三塔
    if (isBig && c1 >= 2) {
        EatCard eat;
        eat.type = eatSanta;
        eat.big = false;
        eat.eatCard = card;
        eat.cards.push_back(temp1);
        eat.cards.push_back(temp1);
        eats.push_back(eat);
    }
    temp2 = Card::makeCard(point, true);
    c2 = __getCardCount(cards,temp2);
    //大三塔
    if (!isBig && c2 >= 2) {
        EatCard eat;
        eat.type = eatSanta;
        eat.big = true;
        eat.eatCard = card;
        eat.cards.push_back(temp2);
        eat.cards.push_back(temp2);
        eats.push_back(eat);
    }
    //大小三塔
    if (c1 + c2 >= 2 &&/* c1 < 3 && c2 < 3 && */c1 > 0 && c2 > 0) {
        EatCard eat;
        eat.type = eatSanta;
        eat.big = isBig;
        eat.eatCard = card;
        eat.cards.push_back(temp1);
        eat.cards.push_back(temp2);
        eats.push_back(eat);
    }
    return !eats.empty();
}

// 从手牌中吃下card
// 参数：
// 	card：待吃的牌
// 	eats：吃掉card后的吃牌
bool HandCards::_checkChi(const Card& card, EATCARDS_t& eats) const
{
	eats.clear();
	int point = card.getPoint();
	bool isBig = card.IsBig();

	//无效牌.数量为0,比如 判断是否能吃 小1,point 为 1, -2 为无效牌.参考Card构造函数.
	//顺子判断
	Card temp1;
	Card temp2;

	temp1 = Card::makeCard(point + 1, isBig);
	temp2 = Card::makeCard(point + 2, isBig);
	int c1 = _getCardCount(temp1);
	int c2 = _getCardCount(temp2);
	if (c1 > 0 && c2 > 0 && c1 < 3 && c2 < 3) {
		EatCard eat;
		eat.type = point == 1 ? eat123 : eatShunZi;
		eat.big = isBig; 
		eat.eatCard = card;
		eat.cards.push_back(temp1);
		eat.cards.push_back(temp2);
		eats.push_back(eat);
	}
	temp2 = Card::makeCard(point - 1, isBig);
	c2 = _getCardCount(temp2);
	if (c1 > 0 && c2 > 0 && c1 < 3 && c2 < 3) {
		EatCard eat;
		eat.type = point == 2 ? eat123 : eatShunZi;
		eat.big = isBig;
		eat.eatCard = card;
		eat.cards.push_back(temp2);
		eat.cards.push_back(temp1);
		eats.push_back(eat);
	}

	temp1 = Card::makeCard(point - 2, isBig);
	c1 = _getCardCount(temp1);
	if (c2 > 0 && c1 > 0 && c2 < 3 && c1 < 3) {
		EatCard eat;
		eat.type = point == 3 ? eat123 : eatShunZi;
		eat.big = isBig;
		eat.eatCard = card;
		eat.cards.push_back(temp1);
		eat.cards.push_back(temp2);
		eats.push_back(eat);
	}

	//三塔判断
	temp1 = Card::makeCard(point, false);
	c1 = _getCardCount(temp1);
	//小三塔
	if (isBig && c1 == 2) {
		EatCard eat;
		eat.type = eatSanta;
		eat.big = false;
		eat.eatCard = card;
		eat.cards.push_back(temp1);
		eat.cards.push_back(temp1);
		eats.push_back(eat);
	}
	temp2 = Card::makeCard(point, true);
	c2 = _getCardCount(temp2);
	//大三塔
	if (!isBig && c2 == 2) {
		EatCard eat;
		eat.type = eatSanta;
		eat.big = true;
		eat.eatCard = card;
		eat.cards.push_back(temp2);
		eat.cards.push_back(temp2);
		eats.push_back(eat);
	}
	//大小三塔
	if (c1 + c2 >= 2 && c1 < 3 && c2 < 3 && c1 > 0 && c2 > 0) {
		EatCard eat;
		eat.type = eatSanta;
		eat.big = isBig;
		eat.eatCard = card;
		eat.cards.push_back(temp1);
		eat.cards.push_back(temp2);
		eats.push_back(eat);
	}

	//2 7 10 判断
	if (point == 2) {
		temp1 = Card::makeCard(7, isBig);
		temp2 = Card::makeCard(10, isBig);
		c1 = _getCardCount(temp1);
		c2 = _getCardCount(temp2);
		if (c1 > 0 && c2 > 0 && c1 < 3 && c2 < 3) {
			EatCard eat;
			eat.type = eatHong;
			eat.big = isBig;
			eat.eatCard = card;
			eat.cards.push_back(temp1);
			eat.cards.push_back(temp2);
			eats.push_back(eat);
		}
	}
	else if (point == 7) {
		temp1 = Card::makeCard(2, isBig);
		temp2 = Card::makeCard(10, isBig);
		c1 = _getCardCount(temp1);
		c2 = _getCardCount(temp2);
		if (c1 > 0 && c2 > 0 && c1 < 3 && c2 < 3) {
			EatCard eat;
			eat.type = eatHong;
			eat.big = isBig;
			eat.eatCard = card;
			eat.cards.push_back(temp1);
			eat.cards.push_back(temp2);
			eats.push_back(eat);
		}
	}
	else if (point == 10) {
		temp1 = Card::makeCard(2, isBig);
		temp2 = Card::makeCard(7, isBig);
		c1 = _getCardCount(temp1);
		c2 = _getCardCount(temp2);
		if (c1 > 0 && c2 > 0 && c1 < 3 && c2 < 3) {
			EatCard eat;
			eat.type = eatHong;
			eat.big = isBig;
			eat.eatCard = card;
			eat.cards.push_back(temp1);
			eat.cards.push_back(temp2);
			eats.push_back(eat);
		}
	}

	return !eats.empty();
}


// cardNum 为1~2
bool HandCards::_checkBi(const Card& card, EatChiCard& eatChi, int cardNum)
{
	if (eatChi.eats.size() == 0) return false;

	_optCardCount(card, -1);

	EATCARDS_t::iterator iter = eatChi.eats.begin();

	int idx = 0;
	for (; iter != eatChi.eats.end();) {
		for (int i = 0; i < (int)iter->cards.size(); ++i) {
			_optCardCount(iter->cards[i], -1);
		}

		EATCARDS_t eats;
		_EATBI_t   eatBi;
		int cut = _getCardCount(card); // cardNum==1时，cut==-1~0，cardNum==2时，cut==0~1

		if (cut >= 0) {
			_checkChi(card, eats);
			if (cardNum == 2) {
				EATCARDS_t eats2;
				EATCARDS_t::iterator it = eats.begin();
				int index = 0;
				for (; it != eats.end();) {
					for (int i = 0; i < (int)it->cards.size(); ++i) {
						_optCardCount(it->cards[i], -1);
					}
					int curCard = _getCardCount(card);
					if (curCard > 0) {
						_optCardCount(card, -1);
						_checkChi(card, eats2);
						_optCardCount(card, 1);
					}
					for (int i = 0; i < (int)it->cards.size(); ++i) {
						_optCardCount(it->cards[i], 1);
					}

					if (eats2.size() == 0 && curCard > 0) {
						it = eats.erase(it);
						continue;
					}
					else if (eats2.size() == 0) {
						++it;
						++index;
						continue;
					}
					else {
						eatBi.push_back(_EatBi());
						_EatBi& temp = eatBi.back();
						temp.idx = index;
						temp.cats.swap(eats2);
						++it;
						++index;
					}
				}
			}
		}
		
		for (int i = 0; i < (int)iter->cards.size(); ++i) {
			_optCardCount(iter->cards[i], 1);
		}

		if (eats.size() == 0 && cut >= 0) {
			iter = eatChi.eats.erase(iter);
			continue;
		}
		else if (eats.size() == 0) {
			++iter;
			++idx;
			continue;
		}else{
			for (int i = 0; i < (int)eats.size(); ++i) {
				_EatBi temp;
				temp.idx = idx;
				temp.cats.push_back(eats[i]);
				for (int j = 0; j < (int)eatBi.size(); ++j) {
					if (eatBi[j].idx == i) {
						std::copy(eatBi[j].cats.begin(), eatBi[j].cats.end(), std::back_inserter(temp.cats));
					}
				}
				eatChi.bi.push_back(temp);
			}
			++idx;
			++iter;
		}
	}
	_optCardCount(card, 1);
	return eatChi.eats.size() > 0;
}

bool HandCards::_checkPaoHu(const Card& card, bool self, bool fetch)
{
	if (checkPao(card, !fetch)) {
		// 先判断跑后能不能胡，胡息多
		EATCARDS_t eatCards;
		eatCards = _eatCards;
		int8_t cards[20];
		memcpy(cards, _cards, sizeof(cards));
		// 吃跑会修改以下变量，如果跑后不能胡，需要还原回去
		uint8_t		bigCards = _bigCards;	//大字
		uint8_t		smallCards = _smallCards;
		uint8_t		redCards = _redCards;	//红字
		uint8_t		paoTimes = _paoTimes;	//跑牌次数.
		uint8_t		huXi = _huXi;
		uint8_t		huXi2 = _huXi2;
		uint8_t		countCards = _countCards;
		if (!EatPao(card)) 
			return false;
        if (checkPaoFlag()) return false;

		bool result = _checkHu(card, self, fetch, true);

		_eatCards.swap(eatCards);
		memcpy(_cards, cards, sizeof(cards));
		_bigCards = bigCards;
		_smallCards = smallCards;
		_redCards = redCards;
		_paoTimes = paoTimes;
		_huXi = huXi;
		_huXi2 = huXi2;
		_countCards = countCards;
        return result;
	}
    return false;
}

bool HandCards::_checkHu(const Card& card, bool self, bool fetch, bool paohu)
{
	int xi = _huXi;
    _huCard = card;

	// 把单牌和对子拎出来
	EATCARDS_t eats;
    eats.clear();

	int8_t _tmpCards[20];
	memcpy(_tmpCards, _cards, sizeof(_tmpCards));

	for (int i = 0; i < 20; ++i) {
		int cardNum = _tmpCards[i];
		if (cardNum == 0) continue;

		int cdVal = i + 1 - (i >= 10 ? 10 : 0);
		Card temp = Card::makeCard(cdVal, i >= 10);
		if (cardNum == 3) {
			EatCard eat;
			xi += (i >= 10) ? 6 : 3;
			eat.type = eatKan;
			eat.eatCard = temp;
			eat.big = temp.IsBig();
			eats.push_back(eat);
			_tmpCards[i] = 0;
		}
	}

    int cardXi = 0;
    EATCARDS_t _eats;
	if (__getCardCount(_tmpCards, card) == 2) {
		__setCardCount(_tmpCards, card, 0);
        EATCARDS_t _eats = eats;

        EatCard eat;
        eat.type = eatPeng;
        eat.eatCard = card;
        eat.big = card.IsBig();
        _eats.push_back(eat);
        cardXi = card.IsBig() ? 3 : 1;

        __checkHu(_tmpCards, xi + cardXi, _eats);
        __setCardCount(_tmpCards, card, 2);
    }

    cardXi = 0;
    if (!paohu) {
        __optCardCount(_tmpCards, card, 1);
        __checkHu(_tmpCards, xi + cardXi, eats);
        __optCardCount(_tmpCards, card, -1);
    }
    else {
        __checkHu(_tmpCards, xi + cardXi, eats);
    }
    return !_eatHuCardList.empty();
}

// 参数：
//  cards: 当前手牌
// 	xi：判断前已有的息
bool HandCards::__checkHu(int8_t cards[20], int xi, EATCARDS_t& eats)
{
	MAP_CARDS_t dui; // 对牌集合
	MAP_CARDS_t dan; // 单牌集合
	for (int i = 0; i < 20; ++i) {
		int cardNum = cards[i];
		if (cardNum == 0) continue;

		int cdVal = i + 1 - (i >= 10 ? 10 : 0);
		Card temp = Card::makeCard(cdVal, i >= 10);

		if (cardNum == 1) {
			dan.insert(std::make_pair(temp.getVal(), cardNum));
		}
		else if (cardNum == 2) {
			dui.insert(std::make_pair(temp.getVal(), cardNum));
		}
        else if (cardNum == 3) {
            dan.insert(std::make_pair(temp.getVal(), 1));
            dui.insert(std::make_pair(temp.getVal(), 2));
        }
	}

	int cardXi = 0;

	// 吃掉所有单牌
	for (MAP_CARDS_t::const_iterator iter = dan.begin(); iter != dan.end(); ++iter) {
		if (iter->second == 0)
			continue;
		int val = iter->first;

		Card temp(val);
		EATCARDS_t cceats;
		__optCardCount(cards, temp, -1);
		// 此时手牌cards中没有单牌temp
		if (__checkChi(cards, temp, cceats)) {
			for (EATCARDS_t::iterator it = cceats.begin(); it != cceats.end(); ++it) {
				EatCard& eat = *it;

				const Card& card1 = eat.cards[0];
				const Card& card2 = eat.cards[1];

				__optCardCount(cards, card1, -1);
				__optCardCount(cards, card2, -1);

				// 吃牌算息
				cardXi = (eat.type == eatHong || eat.type == eat123) ? (temp.IsBig() ? 6 : 3) : 0;

                EATCARDS_t _eats;
                std::copy(eats.begin(), eats.end(), std::back_inserter(_eats));
                _eats.push_back(eat);
                __checkHu(cards, xi + cardXi, _eats);

                __optCardCount(cards, card1, 1);
                __optCardCount(cards, card2, 1);
			}
			// 所有吃掉单牌的方案都不能胡，则不能胡了
			__optCardCount(cards, temp, 1);
			return false;
		}
		__optCardCount(cards, temp, 1);
		// 存在单牌吃不掉，则不能胡了 返回false TODO
	}
	if (dan.size() > 0) 
		return false;
	// 单牌已经吃完

	if (dan.size() > 0) 
        return false;

	if (dui.size() > 1) {
		//对子还有多余的.是否能组成顺子.
		for (MAP_CARDS_t::const_iterator iter = dui.begin(); iter != dui.end(); ++iter) {
			int val = iter->first;

			Card temp(val);
			__optCardCount(cards, temp, -1); // 先提一张出去，再吃进来
			EATCARDS_t cceats;
			if (__checkChi(cards, temp, cceats)) {
				for (EATCARDS_t::iterator it = cceats.begin(); it != cceats.end(); ++it) {
					EatCard& eat = *it;

                    const Card& card1 = eat.cards[0];
                    const Card& card2 = eat.cards[1];

					__optCardCount(cards, card1, -1);
					__optCardCount(cards, card2, -1);
					//红字算息
					cardXi = (eat.type == eatHong || eat.type == eat123) ? (temp.IsBig() ? 6 : 3) : 0;

                    EATCARDS_t _eats;
                    std::copy(eats.begin(), eats.end(), std::back_inserter(_eats));
                    _eats.push_back(eat);
                    __checkHu(cards, xi + cardXi, _eats);

                    __optCardCount(cards, card1, 1);
                    __optCardCount(cards, card2, 1);
				}
			}
			__optCardCount(cards, temp, 1);
		}
		return false;
	}
	
	if (xi < __NEED_HUXI)
		// 胡息不够
		return false;

	if(dui.size() == 1){
		// 没跑、提手中不能有对.
		if (_paoTimes == 0)
			return false;
		Card temp(dui.begin()->first);
		EatCard eat;
		eat.type = eatDui;
		eat.eatCard = temp;
		eat.big = temp.IsBig();
		eats.push_back(eat);
	}

    EatHuCards eatHuCards;
    eatHuCards.xi = xi;
    eatHuCards.huCards = eats;
    _eatHuCardList.push_back(eatHuCards);
    return true;
}

int HandCards::qishouTiCardSize() const
{
    int ret = 0;
    for (EATCARDS_t::const_iterator it = _eatCards.begin(); it != _eatCards.end(); ++it) {
        if (it->type == eatQishouTi)
            ++ret;
    }
    return ret;
}

bool HandCards::__isMonkey() const
{
    return _countCards == 1;
}

void HandCards::getCardFlags(uint8_t cardFlags[20]) const
{
	memcpy(cardFlags, _cardFlags, sizeof(_cardFlags));
}

void HandCards::setCardFlags(uint8_t cardFlags[20])
{
	memcpy(_cardFlags, cardFlags, sizeof(_cardFlags));
}

uint8_t HandCards::getFlags(void) const
{
	return _flags;
}

void HandCards::setFlags(uint8_t flags)
{
	_flags = flags;
}

bool HandCards::__isHong(const Card& card)
{
	int ptVal = card.getPoint();
	return ptVal == 2 || ptVal == 7 || ptVal == 10;
}

int HandCards::_getIndex(const Card& card)
{
	int point = card.getPoint();
	bool isBig = card.IsBig();
	int idx = point - 1;
	if (isBig) idx += 10;
	return idx;
}

