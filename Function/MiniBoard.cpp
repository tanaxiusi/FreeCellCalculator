#include "MiniBoard.h"
#include <memory.h>

MiniBoard::MiniBoard()
{
	memset(data, 0, sizeof(data));
}

MiniBoard::MiniBoard( const Board & b )
{
	memset(data, 0, sizeof(data));
	for(int i = 0; i < 4; i++)
	{
		if(qint8 cardIndex = b.aboveLeft[i].nativeIndex())
			data[cardIndex - 1] = i + 1;

		if(qint8 cardIndex = b.aboveRight[i].nativeIndex())
			data[cardIndex - 1] = i + 5;
	}
	for(int i = 0; i < 8; i++)
	{
		for(int j = 0; j < MaxCardCount; j++)
		{
			qint8 cardIndex = b.below[i][j].nativeIndex();
			if(cardIndex == 0)
				break;
			data[cardIndex - 1] = 9 + i * MaxCardCount + j;
		}
	}
}

MiniBoard::operator Board() const
{
	Board b;

	for(char i = 0; i < 52; i++)
	{
		unsigned char pos = data[i];
		char cardIndex = i + 1;

		if(pos >= 1 && pos <= 4)
		{
			b.aboveLeft[pos - 1] = Card::fromNativeIndex(cardIndex);
		}else if(pos >= 5 && pos <=8)
		{
			b.aboveRight[pos - 5] = Card::fromNativeIndex(cardIndex);
		}else if(pos >= 9 && pos <= 168)
		{
			b.below[(pos - 9) / MaxCardCount][(pos - 9) % MaxCardCount] = Card::fromNativeIndex(cardIndex);
		}
	}
	return b;
}

bool MiniBoard::operator<( const MiniBoard & other ) const
{
	return memcmp(this, &other, sizeof(MiniBoard)) < 0;
}

bool MiniBoard::operator==( const MiniBoard & other ) const
{
	return memcmp(this, &other, sizeof(MiniBoard)) == 0;
}

bool MiniBoard::operator!=( const MiniBoard & other ) const
{
	return memcmp(this, &other, sizeof(MiniBoard)) != 0;
}
