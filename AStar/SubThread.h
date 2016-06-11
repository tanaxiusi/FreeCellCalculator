#pragma once

#include <QThread>
#include <QRunnable>
#include <QMutex>
#include "WinReadWriteLockPool.h"
#include "AreaBase.h"
#include "AHash.h"

namespace AStar
{

	template<class Block>
	class Node
	{
	public:
		Block parent;
		double G;
		double H;
		bool isClose;
	public:
		Node() { G = H = 0; isClose = false; }
		double F() { return G + H; }
	};

	template<class Block>
	struct ThreadSharedData
	{
	public:
		const AreaBase<Block> * const area;
		const Block begin;
		const Block end;
		const int threadCount;
		bool pathFound = false;
		bool finished = false;
		QAtomicInt waitingThreadCount = 0;

		AHash<Block, Node<Block> > mapAll;
		std::multimap<double, Block> mapOpen;
		typedef typename std::multimap<double, Block>::iterator MultiMapIterator;
		std::map<Block, MultiMapIterator> mapOpen_reversed;

		QMutex lockForOpen;

		QAtomicInt wasted = 0;

	public:
		ThreadSharedData(const AreaBase<Block> * const area, const Block & begin, const Block & end, int threadCount)
			:area(area), begin(begin), end(end), threadCount(threadCount)
		{
		}
		WinReadWriteLock * lockForSingle(const Block & block)
		{
			return m_lockPool.get(qHash(block));
		}

	private:
		WinReadWriteLockPool m_lockPool;

	};

	template<class Block>
	class SubThread : public QRunnable
	{
	public:
		SubThread(ThreadSharedData<Block> * sharedData, int index)
		{
			this->sharedData = sharedData;
			this->index = index;
		}

	protected:
		virtual void run();

	private:
		ThreadSharedData<Block> * sharedData;
		int index;
	public:

		int try1 = 0;
		int try2 = 0;
		int try3 = 0;
		int try4 = 0;
		int try5 = 0;
		int try6 = 0;
		int try7 = 0;
		int try8 = 0;
		int try9 = 0;
	};

	template<class Block>
	void AStar::SubThread<Block>::run()
	{
		//SetThreadAffinityMask(GetCurrentThread(), 1 << index);
		ThreadSharedData<Block> & d = *sharedData;

		std::vector<Block> aroundBlock;
		std::vector<double> aroundG;

		bool waiting = false;

		while (true)
		{
			if(d.finished)
				break;

			Block current;

			{
				while (!d.lockForOpen.tryLock())
					try1++;

				if (d.mapOpen.size() == 0)
				{
					d.lockForOpen.unlock();
					if (!waiting)
					{
						waiting = true;
						const int waitingCount = d.waitingThreadCount.fetchAndAddRelaxed(1) + 1;
						Q_ASSERT(waitingCount > 0 && waitingCount <= d.threadCount);
						// 全部线程都陷入等待，无法继续进行
						if (waitingCount == d.threadCount)
							d.finished = true;
					}

					//QThread::yieldCurrentThread();
					d.wasted.fetchAndAddRelaxed(1);
					continue;
				}

				if (waiting)
				{
					waiting = false;
					const int waitingCount = d.waitingThreadCount.fetchAndAddRelaxed(-1) - 1;
					Q_ASSERT(waitingCount >= 0 && waitingCount < d.threadCount);
				}

				// 抢一个估计距离最小的open点，本次处理
				std::multimap<double, Block>::iterator iterNext = d.mapOpen.begin();
				current = iterNext->second;

				d.mapOpen_reversed.erase(current);
				d.mapOpen.erase(iterNext);

				d.lockForOpen.unlock();
			}

			// node标为close
			WinReadWriteLock * lockSingleCurrent(d.lockForSingle(current));
			while(!lockSingleCurrent->tryLockForWrite())
				try3++;

			AHash<Block, Node<Block> >::iterator iterCurrent = d.mapAll.atomicFind(current);
			Q_ASSERT(iterCurrent != d.mapAll.end());
			Node<Block> * nodeCurrent = &iterCurrent.value();
			nodeCurrent->isClose = true;
			const double parentG = nodeCurrent->G;

			lockSingleCurrent->unlock();

			nodeCurrent = NULL;

			if (current == d.end)
			{
				d.pathFound = true;
				d.finished = true;
				break;
			}

			aroundBlock.clear();
			aroundG.clear();

			d.area->GetAroundBlock(current, aroundBlock, aroundG);
			
			for (int i = 0; i < (int)aroundBlock.size(); i++)
			{
				const Block & around = aroundBlock[i];

				if(d.area->GetBlockStatus(around) == BlockStatus_NoPass)
					continue;

				double thisG;
				if (i < (int)aroundG.size())
					thisG = parentG + aroundG[i];
				else
					thisG = parentG + d.area->FuncG(current, around);

				bool ignore = false;

				AHash<Block, Node<Block> >::iterator iterAround = d.mapAll.atomicFind(around);
				bool exist = iterAround != d.mapAll.end();

				if (exist)
				{
					Node<Block> * nodeAround = &iterAround.value();
					WinReadWriteLock * lockForSingleAround = d.lockForSingle(around);
					while(!lockForSingleAround->tryLockForRead())
						try5++;
					// around节点已经close，或者open但是从current节点到该around节点并不会更短，忽略
					if (nodeAround->isClose || thisG >= nodeAround->G)
						ignore = true;
					lockForSingleAround->unlock();
				}

				if (!ignore)
				{
					// 不忽略，要么该around节点还不存在(!exist)，要么exist&&open且上面扫描发现从current走更短
					Node<Block> * nodeAround;

					WinReadWriteLock * lockSingleAround = d.lockForSingle(around);

					if (exist)
					{
						nodeAround = &iterAround.value();
						while(!lockSingleAround->tryLockForWrite())
							try6++;
					}
					else
					{
						// 上面扫描发现不存在，但是这里仍然有可能被其他线程抢先创建了
						while(!lockSingleAround->tryLockForWrite())
							try8++;
						nodeAround = &d.mapAll.atomicCreate(around, Node<Block>(), false).value();
					}

					// 这里还要检查一下isClose，还为open的话再继续判断修改G，如果已经close就说明被其他线程抢去作current了
					if (nodeAround->isClose)
						lockSingleAround->unlock();
					else
					{
						const bool isNew = (nodeAround->G == 0);
						if(!exist && !isNew)
							d.wasted.fetchAndAddRelaxed(1);
						if (isNew || thisG < nodeAround->G)
						{
							if(isNew)
								nodeAround->H = d.area->FuncH(around, d.end, d.begin);
							// 如果新距离比较小，就修改父节点
							nodeAround->parent = current;
							nodeAround->G = thisG;
						}
						lockSingleAround->unlock();

						while(!d.lockForOpen.tryLock())
							try9++;
						if(!isNew)
						{
							auto iterOpenReversed = d.mapOpen_reversed.find(around);
							if (iterOpenReversed != d.mapOpen_reversed.end())
							{
								d.mapOpen.erase(iterOpenReversed->second);
								iterOpenReversed->second = d.mapOpen.insert(std::make_pair(nodeAround->F(), around));
							}
							else
							{
								d.wasted.fetchAndAddRelaxed(1);
								// 上面lockForOpen.lockForWrite之前，这个around节点被其他线程抢去当作current了，就不要再加入open了……
							}
						}
						else
						{
							auto iterOpen = d.mapOpen.insert(std::make_pair(nodeAround->F(), around));
							Q_ASSERT(d.mapOpen_reversed.find(around) == d.mapOpen_reversed.end());
							d.mapOpen_reversed[around] = iterOpen;
						}
						d.lockForOpen.unlock();
					}
				}
			}
		}
	}

}