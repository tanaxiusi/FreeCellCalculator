#pragma once
#include "AStar/AreaBase.h"
#include "Board.h"
#include "MiniBoard.h"


/// BoardArea是游戏状态(MiniBoard)的一个合集，类似于AStar寻路中的地图，每种游戏状态是一个节点
class BoardArea : public AStar::AreaBase<MiniBoard>
{
	typedef MiniBoard _BaseType;
public:
	// GetAroundBlock：获取周围的块(获取该游戏状态下一步可能的走法)
	virtual void GetAroundBlock(const _BaseType & miniBoard, std::vector<_BaseType> & vecOutBlock, std::vector<double> & vecOutG) const;
	// GetBlockStatus：获取块状态(这里始终为"通路")
	virtual int GetBlockStatus(const _BaseType & miniBoard) const;
	// FuncG：相邻两块移动代价(这里始终为固定值)
	virtual double FuncG(const _BaseType & miniBoard1, const _BaseType & miniBoard2) const;
	// FuncH：任意两块间的估计移动代价
	virtual double FuncH(const _BaseType & miniBoard1, const _BaseType & miniBoardEnd, const _BaseType & miniBoardBegin) const;
};