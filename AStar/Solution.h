#pragma once
#include <vector>
#include <map>
#include "AreaBase.h"

namespace AStar
{
	class ProgressReceiver
	{
	public:
		virtual void onProgressChange(bool isFinish, int openCount, int totalCount) = 0;
		virtual bool isAbort() = 0;
	};

	
	template<class BlockType>
	class Node
	{
	public:
		BlockType parent;
		double G;
		double H;
		char isClose;
	public:
		Node(){G = H = 0; isClose = false;}
		double F(){return G + H;}
	};


	template<class BlockType>
	void GetPath(const AreaBase<BlockType> * pArea, const BlockType & begin, const BlockType & end, std::vector<BlockType> & outPath, ProgressReceiver * pReceiver = 0)
	{
		int counter = 0;


		// mapAllNode：存储所有节点信息
		typename std::map<BlockType, Node<BlockType> > mapAllNode;

		// mapOpen：open节点列表，first为F值，使用multimap以快速找到最小值
		std::multimap<double, BlockType> mapOpen;
		// mapBlock2Iter：内容与mapOpen一致，用于快速反向查询
		typedef typename std::multimap<double, BlockType>::iterator MultiMapIterator;
		std::map<BlockType, MultiMapIterator> mapBlock2Iter;

		BlockType ptBegin = begin;
		BlockType ptEnd = end;
		BlockType ptNow = begin;

		Node<BlockType> * pNodeNow = &mapAllNode[ptNow];
		pNodeNow->parent = ptNow;

		bool pathFound = false;

		std::vector<BlockType> vecAround;
		std::vector<double> vecG;

		while(!pathFound)
		{
			
			// 获取now周围点
			double parentG = pNodeNow->G;
			vecAround.clear();
			vecG.clear();

			pArea->GetAroundBlock(ptNow, vecAround, vecG);

			for(int i = 0; i < (int)vecAround.size(); i++)
			{
				const BlockType & pt = vecAround[i];

				Node<BlockType> * pNode = &mapAllNode[pt];

				if(pArea->GetBlockStatus(pt) != BlockStatus_NoPass &&
					false == pNode->isClose)
				{
					// 如果该点不是障碍物，且未被处理过，计算从起点到该点的距离
					double thisG;

					if(i < (int)vecG.size())
						thisG = parentG + vecG[i];
					else
						thisG = parentG + pArea->FuncG(ptNow, pt);

					typename std::map<BlockType, MultiMapIterator>::iterator itMapIter = mapBlock2Iter.find(pt);

					if(itMapIter == mapBlock2Iter.end())
					{
						// 如果该点还没被计算距离，则加入open列表，该点将在下一次扫描时处理
						// 设置父节点
						pNode->parent = ptNow;
						pNode->G = thisG;
						pNode->H = pArea->FuncH(pt, ptEnd, ptBegin);

						mapBlock2Iter[pt] = mapOpen.insert(typename std::pair<double, BlockType>(pNode->F(), pt));

					}else
					{
						
						// 该点已经计算过距离
						if(thisG < pNode->G)
						{
							// 如果新距离比较小，就修改父节点
							pNode->parent = ptNow;
							pNode->G = thisG;

							mapOpen.erase(itMapIter->second);
							itMapIter->second = mapOpen.insert(typename std::pair<double, BlockType>(pNode->F(), pt));
						}
					}
				}
			}

			// 把当前点设为关闭状态
			pNodeNow->isClose = true;
			
			if(mapBlock2Iter.size() == 0)
				break;

			{
				// 找出估计距离最小的open点，下次处理
				MultiMapIterator itBegin = mapOpen.begin();
				BlockType ptNext = itBegin->second;

				// 把下次处理的点从open列表中移除
				mapBlock2Iter.erase(ptNext);
				mapOpen.erase(itBegin);

				ptNow = ptNext;
			}

			if(ptNow == ptEnd)
			{
				pathFound = true;
			}

			pNodeNow = &mapAllNode[ptNow];

			if(pReceiver && pReceiver->isAbort())
				break;

			counter++;

			if(counter == 2000)
			{
				counter = 0;
				if(pReceiver)
					pReceiver->onProgressChange(false, mapOpen.size(), mapAllNode.size());
			}		
		}

		if(pathFound && !(pReceiver && pReceiver->isAbort()))
		{
			outPath.insert(outPath.begin(), 1, ptEnd);

			BlockType pt = ptEnd;
			BlockType ptPrev = mapAllNode[pt].parent;
			while(ptPrev != pt)
			{
				outPath.insert(outPath.begin(), 1, ptPrev);
				pt = ptPrev;
				ptPrev = mapAllNode[pt].parent;
			}
		}

		if(pReceiver)
			pReceiver->onProgressChange(true, mapOpen.size(), mapAllNode.size());
	}
}