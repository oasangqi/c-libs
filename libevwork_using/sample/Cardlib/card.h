#pragma once

#include <vector>


/* 字牌类.
	由 0x01 - 0x0A 分别标识 小牌 1-10
	由 0x11 - 0x1A 分别标识 大牌 1-10
*/

class Card
{
public:
	Card() 
		:_val(0)
	{ 
	}
	~Card() 
	{
	}
	Card(const Card& rhs) 
	{
		_val = rhs._val;
	}
	Card(int val)
		:_val(val)
	{
		if (!IsCard(val))
			_val = 0;
	}

	int		getPoint(void) const
	{
		if (!IsValid()) return 0;
		return _val & 0x0F;
	}

	//判断是否为大字.未检测是否为有效牌.
	bool	IsBig(void) const
	{
		return (_val >> 4) > 0;
	}

	//判断是否为小字.未检测是否为有效牌.
	bool	IsSmall(void) const
	{
		return !IsBig();
	}

	bool	IsXiCard(void) const
	{
		int p = getPoint();

		if (p == 1 || p == 2 || p == 3 || p == 7 || p == 10) {
			return true;
		}
		return false;
	}

	bool	IsValid(void) const
	{
		return _val != 0;
	}

	void	Reset(void) 
	{
		_val = 0;
	}

	//取代表牌的值.
	int		getVal(void) const
	{
		return _val;
	}

	bool	operator < (const Card& rhs) const
	{
		bool big = IsBig();
		bool rBig = rhs.IsBig();
		if (big && !rBig) return false;
		else if (!big && rBig) return true;
		return _val < rhs._val;
	}

	bool	operator == (const Card& rhs) const
	{
		return _val == rhs._val;
	}

	bool	operator !=(const Card& rhs) const
	{
		return _val != rhs._val;
	}

	Card	operator + (int n) const
	{
		return Card(_val + n);
	}

	Card	operator - (int n) const
	{
		return Card(_val - n);
	}

	Card&	operator = (const Card& rhs) {
		if (&rhs != this) {
			_val = rhs._val;
		}
		return *this;
	}

	static Card makeCard(int point, bool big) 
	{
		int val = point;
		if (big) val += (1 << 4);
		return Card(val);
	}

private:
	static bool	IsCard(int val)
	{
		if (val >= 0x11 && val <= 0x1A)
			return true;
		if (val >= 0x01 && val <= 0x0A)
			return true;
		return false;
	}
private:
	int		_val;
};

typedef std::vector<Card>	CARDS_t;
