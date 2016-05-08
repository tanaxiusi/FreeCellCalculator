#include "Board.h"
#include <string.h>

bool Board::AutoFinishStepByStep = true;

Board Board::GetFinishStatus()
{
	Board ret;
	ret.aboveRight[0] = Card(13, Card::Club);
	ret.aboveRight[1] = Card(13, Card::Diamond);
	ret.aboveRight[2] = Card(13, Card::Heart);
	ret.aboveRight[3] = Card(13, Card::Spade);

	return ret;
}


Board::Board()
{
	memset(this, 0, sizeof(Board));
}

bool Board::operator==( const Board & other ) const
{
	return memcmp(this, &other, sizeof(Board)) == 0;
}

bool Board::operator!=( const Board & other ) const
{
	return memcmp(this, &other, sizeof(Board)) != 0;
}

bool Board::operator<( const Board & other ) const
{
	return memcmp(this, &other, sizeof(Board)) < 0;
}

int Board::GetBelowCount( int column ) const
{
	if(column >= 0 && column < 8)
	{
		for(int i = 0; i < MaxCardCount; ++i)
			if(!below[column][i].isValid())
				return i;
		return MaxCardCount;
	}
	return 0;
}

int Board::GetFinishCount() const
{
	return aboveRight[0].number() + aboveRight[1].number() + aboveRight[2].number() + aboveRight[3].number();
}


Card Board::GetBelow( int column, int index ) const
{
	if(column < 0 || column >= 8 || index < 0)
		return Card::Invalid;

	int count = GetBelowCount(column);
	if(count > 0 && count > index)
		return below[column][count - 1 - index];
	else
		return Card::Invalid;

}


Card Board::GetAboveLeft( int index ) const
{
	if(index >= 0 && index < 4)
		return aboveLeft[index];
	else
		return Card::Invalid;
}

Card Board::GetAboveRight(Card::Shape shape) const
{
	if(shape >= 0 && shape < 4)
		return aboveRight[shape];
	else
		return Card::Invalid;
}



Card Board::PopAboveLeft( int index )
{
	Card card = GetAboveLeft(index);
	if(card.isValid())
	{
		aboveLeft[index] = Card::Invalid;
		return card;
	}

	return Card::Invalid;
}

Card Board::PopBelow( int column )
{
	if(column < 0 || column >= 8)
		return Card::Invalid;

	if(int count = GetBelowCount(column))
	{
		Card card = below[column][count - 1];
		below[column][count - 1] = Card::Invalid;
		return card;
	}

	return Card::Invalid;
}

bool Board::PushAboveLeft( Card card )
{
	for(int i = 0; i < 4; i++)
	{
		if(!aboveLeft[i].isValid())
		{
			aboveLeft[i] = card;
			return true;
		}
	}

	return false;
}


bool Board::PushAboveRight( Card card )
{
	const int i = (int)card.shape();

	Card & current = aboveRight[i];
	if(!current.isValid() && card.number() == 1 || current.isValid() && current.number() + 1 == card.number())
	{
		current = card;
		return true;
	}

	return false;
}


bool Board::PushBelow( int column, Card card )
{
	if(!card.isValid())
		return false;

	if(column < 0 || column >= 8)
		return false;

	int count = GetBelowCount(column);
	if(count + 1 >= MaxCardCount)
		return false;

	if(count == 0 ||
		below[column][count - 1].color() != card.color() &&
		below[column][count - 1].number() == card.number() + 1)
	{
		below[column][count] = card;
		return true;
	}else
	{
		return false;
	}
}



void Board::AutoFinish()
{
	bool haveMove = false;
 	Card OriAboveRight[4];
 	for(int i = 0; i < 4; i++)
 		OriAboveRight[i] = aboveRight[i];

	Card * myAboveRight = AutoFinishStepByStep ? OriAboveRight : aboveRight;

	while(true)
	{
		haveMove = false;
		// 扫描下方8列和左上4格
		for(int i = 0; i < 12; i++)
		{
			Card card;

			if(i < 8)
				card = GetBelow(i);
			else
				card = GetAboveLeft(i - 8);

			if(!card.isValid())
				continue;
			
			Card::Shape thisShape = card.shape();
			Card::Color thisColor = card.color();
			qint8 thisNumber = card.number();

			// 检测是否放得上去
			if(myAboveRight[thisShape].number() + 1 != thisNumber)
				continue;

			// 与该卡牌颜色不同且点数小一的两张牌是否已经移到右上，这时才能放心地把这张牌移动到右上
			bool canMove = false;

			if(thisNumber > 2)
			{
				char alreadyUp = 0;
				for(int j = 0; j < 4; j++)
				{
					// 颜色相同的过滤
					Card::Shape oppositeShape = (Card::Shape)j;
					if(Card::color(oppositeShape) == thisColor)
						continue;

					// 获取该形状已经完成的点数
					char oppositeNumber = myAboveRight[oppositeShape].number();
					

					if(oppositeNumber >= thisNumber - 1)
						alreadyUp++;
				}
				if(alreadyUp >= 2)
				{
					canMove = true;
				}
			}else
			{
				canMove = true;
			}

			if(canMove)
			{
				PushAboveRight(card);

				if(i < 8)
					PopBelow(i);
				else
					PopAboveLeft(i - 8);

				haveMove = true;
			}
		}
		if(!haveMove || AutoFinishStepByStep)
			break;
	}

	haveMove = 0;
}


int Board::GetBelowContinuousCount( int column ) const
{
	if(GetBelowCount(column) == 0)
		return 0;

	int count = 1;

	int index = 0;
	Card current = GetBelow(column, index);
	Card next = GetBelow(column, index + 1);

	while(current.isValid() && next.isValid() && current.number() + 1 == next.number() && current.color() != next.color())
	{
		count++;
		index++;
		current = next;
		next = GetBelow(column, index + 1);
	}

	return count;
}

bool Board::IsValid() const
{
	qint8 cnt[52] = {0};
	for(int i = 0; i < 4; i++)
	{
		Card card = GetAboveLeft(i);
		if(card.isValid())
		{
			cnt[card.nativeIndex() - 1]++;
		}else if(card.nativeIndex() != 0)
		{
			Q_ASSERT(0);
			return false;
		}
	}

	// 右上区域
	for(int i = 0; i < 4; i++)
	{
		Card card = GetAboveRight((Card::Shape)i);
		if(card.isValid())
		{
			qint8 number = card.number();
			Card::Shape shape = card.shape();

			// 右上区域的一张卡牌实际上“压着”其余点数比它小的同花色牌
			cnt[card.nativeIndex() - 1]++;
			while(--number)
				cnt[Card(number,shape).nativeIndex() - 1]++;
		}else if(card.nativeIndex() != 0)
		{
			Q_ASSERT(0);
			return false;
		}
	}

	for(int i = 0; i < 8; i++)
	{
		int len = GetBelowCount(i);
		if(len >= MaxCardCount)
		{
			Q_ASSERT(0);
			return false;
		}
		for(int j = 0; j < len; j++)
		{
			Card card = below[i][j];
			if(card.isValid())
			{
				cnt[card.nativeIndex() - 1]++;
			}else
			{
				Q_ASSERT(0);
				return false;
			}
		}
	}

	for(int i = 0; i < 52; i++)
		if(cnt[i] != 1)
			return false;

	return true;
}
