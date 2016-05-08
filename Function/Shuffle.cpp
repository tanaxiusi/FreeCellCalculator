#include "Shuffle.h"
#include <stdlib.h>

Board Shuffle( int index )
{
	Board ret;

	int deck[52];
	for (int i = 0; i < 52; i++)
		deck[i] = i;

	srand(index);

	int wLeft = 52;
	for (int i = 0; i < 52; i++)
	{
		int j = rand() % wLeft;
		ret.below[(i%8)][i/8] = Card::fromNativeIndex(deck[j] + 1);
		deck[j] = deck[--wLeft];
	}

	return ret;
}

Board ShuffleSpecial( int index )
{
	Board ret;
	if(index == -1)
	{
		for(int i = 0; i < 4; i++)
			for(int j = 0; j < 7; j++)	
				ret.below[i][j] = Card(j * 2 + 1, (Card::Shape)i);

		for(int i = 0; i < 4; i++)
			for(int j = 0; j < 6; j++)	
				ret.below[i + 4][j] = Card(12 - j * 2, (Card::Shape)i);

	}else if(index == -2)
	{
		for(int i = 0; i < 4; i++)
			for(int j = 0; j < 7; j++)
				if(j == 0)
					ret.below[i][j] = Card(1, (Card::Shape)(3 - i));
				else
					ret.below[i][j] = Card(14 - j, (Card::Shape)(3 - i));

		for(int i = 0; i < 4; i++)
			for(int j = 0; j < 6; j++)	
				ret.below[i + 4][j] = Card(7 - j, (Card::Shape)(3 - i));
	}else if(index == -3)
	{
		for(int i = 0; i < 4; i++)
			for(int j = 0; j < 7; j++)
				ret.below[i][j] = Card(13 - j, (Card::Shape)(3 - i));

		for(int i = 0; i < 4; i++)
			for(int j = 0; j < 6; j++)	
				ret.below[i + 4][j] = Card(6 - j, (Card::Shape)(3 - i));
	}else if(index == -4)
	{
		int column = 0;
		int row = 0;
		for(qint8 cardIndex = 52; cardIndex >= 1; cardIndex--)
		{
			ret.below[column][row] = Card::fromNativeIndex(cardIndex);
			row++;
			int rowCount = (column <= 3) ? 7 : 6;
			if(row >= rowCount)
			{
				row = 0;
				column++;
			}
		}
	}

	return ret;

}
