#pragma once
#include <vector>

namespace AStar
{
	const int BlockStatus_Invalid = -1;
	const int BlockStatus_Free = 0;
	const int BlockStatus_NoPass = 1;

	template<class BlockType>
	class AreaBase
	{
	public:
		// GetAroundBlock：获取周围的块
		virtual void GetAroundBlock(const BlockType & b, std::vector<BlockType> & vecOutBlock, std::vector<double> & vecOutG) const = 0;
		// GetBlockStatus：获取块状态
		virtual int GetBlockStatus(const BlockType & b) const = 0;
		// FuncG：相邻两块移动代价
		virtual double FuncG(const BlockType & b1, const BlockType & b2) const = 0;
		// FuncH：任意两块间的估计移动代价
		virtual double FuncH(const BlockType & b1, const BlockType & bEnd, const BlockType & bBegin) const = 0;
	};
}