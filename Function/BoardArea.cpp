#include "BoardArea.h"
#include <QString>


void BoardArea::GetAroundBlock( const _BaseType & miniBoard, std::vector<_BaseType> & vecOutBlock, std::vector<double> & vecOutG ) const
{
	const Board board = miniBoard;
	{
		Board temp = board;
		// 如果该游戏状态存在可以自动完成的步骤，就必须走这一步
		temp.AutoFinish();
		if(temp != board)
		{
			vecOutBlock.push_back(temp);
			vecOutG.push_back(0);
			return;
		}
	}

	// 尝试从下方移到右上
	for(int i = 0; i < 8; i++)
	{
		Board temp = board;
		Card card = temp.PopBelow(i);
		if(card.isValid() && temp.PushAboveRight(card))
		{
			vecOutBlock.push_back(temp);
			vecOutG.push_back(1);
		}
	}

	// 尝试从左上移到右上
	for(int i = 0; i < 4; i++)
	{
		Board temp = board;
		Card card = temp.PopAboveLeft(i);
		if(card.isValid() && temp.PushAboveRight(card))
		{
			vecOutBlock.push_back(temp);
			vecOutG.push_back(1);
		}
	}

	// 尝试从左上移到下方
	for(int i = 0; i < 4; i++)
	{
		Board temp = board;
		Card card = temp.PopAboveLeft(i);
		if(!card.isValid())
			continue;
		for(int j = 0; j < 8; j++)
		{
			Board temp2 = temp;
			if(temp2.PushBelow(j, card))
			{
				vecOutBlock.push_back(temp2);
				vecOutG.push_back(1);
			}
		}
	}

	// 把下方8列移到左上
	for(int i = 0; i < 8; i++)
	{
		Board temp = board;
		Card card = temp.PopBelow(i);
		if(card.isValid() && temp.PushAboveLeft(card))
		{
			vecOutBlock.push_back(temp);
			vecOutG.push_back(1);
		}
	}

	// 列与列之间尝试移动一张牌
	for(int i = 0; i < 8; i++)
	for(int j = 0; j < 8; j++)
	{
		if(i == j)
			continue;

		Board temp = board;
		Card card = temp.PopBelow(i);
		if(card.isValid() && temp.PushBelow(j, card))
		{
			vecOutBlock.push_back(temp);
			vecOutG.push_back(1);
		}
	}

	// 尝试移动多张
	bool isEmptyBelow[8] = {0};
	int emptyCountAboveLeft = 0;
	int emptyCountBelow = 0;
	for(int i = 0; i < 8; i++)
	{
		if(board.GetBelowCount(i) == 0)
		{
			isEmptyBelow[i] = true;
			emptyCountBelow++;
		}
	}
	for(int i = 0; i < 4; i++)
	{
		if(!board.GetAboveLeft(i).isValid())
			emptyCountAboveLeft++;
	}

	for(int srcColumn = 0; srcColumn < 8; srcColumn++)
	{
		// 获取该列连续牌数量
		int continuousCount = board.GetBelowContinuousCount(srcColumn);
		if(continuousCount < 2)
			continue;

		// 对每个目标列
		for(int destColumn = 0; destColumn < 8; destColumn++)
		{
			if(srcColumn == destColumn)
				continue;

			int offset = continuousCount - 1;

			// 2张以上连续牌移到其他列，最大能移动的数量为：(左上角暂存区空位置数量 + 1) × (除目标列外空列数量 + 1)
			const int canMoveCount = isEmptyBelow[destColumn]
				? (emptyCountAboveLeft + 1) * emptyCountBelow
				: (emptyCountAboveLeft + 1) * (emptyCountBelow + 1);

			const int maxOffset = canMoveCount - 1;
			if(offset > maxOffset)
				offset = maxOffset;

			// 从原始列连续最多，且空位能支持的最大位置开始
			for(; offset > 0; offset--)
			{
				// 取出该处的牌
				Card card = board.GetBelow(srcColumn, offset);

				Board temp = board;

				if(!temp.PushBelow(destColumn, card))
					continue;

				// 如果发现可以放进去，检查剩余空位够不够
				int canMoveCount;
				// 如果目标列是空的，用于中转的空位就要减一
				if(isEmptyBelow[destColumn])
					canMoveCount = (emptyCountAboveLeft + 1) * emptyCountBelow;
				else
					canMoveCount = (emptyCountAboveLeft + 1) * (emptyCountBelow + 1);

				int actualMoveCount = offset + 1;

				if(actualMoveCount > canMoveCount)
					continue;


				// 检测通过，移动剩余牌
				for(int k = offset - 1; k >= 0; k--)
					temp.PushBelow(destColumn, temp.GetBelow(srcColumn, k));
				for(int k = 0; k < offset + 1; k++)
					temp.PopBelow(srcColumn);

				vecOutBlock.push_back(temp);
				vecOutG.push_back(1);
			}
		}
	}

}

int BoardArea::GetBlockStatus( const _BaseType & miniBoard ) const
{
	Q_UNUSED(miniBoard);
	return AStar::BlockStatus_Free;
}

double BoardArea::FuncG( const _BaseType & miniBoard1, const _BaseType & miniBoard2 ) const
{
	Q_UNUSED(miniBoard1);
	Q_UNUSED(miniBoard2);
	return 1;
}

double BoardArea::FuncH( const _BaseType & miniBoard, const _BaseType & miniBoardEnd, const _BaseType & miniBoardBegin ) const
{
	Q_UNUSED(miniBoardEnd);
	// 当前状态
	const Board & board = miniBoard;
	// 开始状态
	const Board & boardBegin = miniBoardBegin;

	// 1.diffCount：移开牌的数量
	int diffCount = 0;
	for(int i = 0; i < 8; i++)
	{
		int size = boardBegin.GetBelowCount(i);
		bool haveDiff = false;
		for(int j = 0; j < size; j++)
		{
			if(!haveDiff && boardBegin.below[i][j] != board.below[i][j])
				haveDiff = true;
			if(haveDiff)
				diffCount++;
		}
	}
	
	// 2.nextNumberCost：四种花色右上角回收区下一张牌的开销，该牌被压得越深，开销越大
	double nextNumberCost = 0;

	for(int i = 0; i < 4; i++)
	{
		const Card::Shape shape = (Card::Shape)i;
		Card card = board.GetAboveRight(shape);

		char number;
		if(card.isValid())
			number = card.number();
		else
			number = 0;

		if(number == 13)
			continue;

		Card cardNext = Card(number + 1, shape);

		bool found = false;

		for(int j = 0; j < 8; j++)
		{
			int count = board.GetBelowCount(j);
			for(int k = 0; k < count; k++)
			{
				if(board.GetBelow(j, k) == cardNext)
				{
					nextNumberCost += k;
					found = true;
					break;
				}
			}

			if(found)
				break;
		}
	}

	double finishCountCost = 52 - board.GetFinishCount();

	return (52 - diffCount + finishCountCost + nextNumberCost * 0.4);
}
