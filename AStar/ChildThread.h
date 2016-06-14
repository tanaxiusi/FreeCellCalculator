#pragma once

#include <QThread>
#include <QRunnable>
#include <QMutex>
#include "WinReadWriteLockPool.h"
#include "AreaBase.h"
#include "AtomicHash.h"

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
		inline double F() { return G + H; }
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

		AtomicHash<Block, Node<Block> > mapAll;
		std::multimap<double, Block> mapOpen;
		typedef typename std::multimap<double, Block>::iterator MultiMapIterator;
		QHash<Block, MultiMapIterator> mapOpen_reversed;

		QMutex lockForOpen;

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
	class ChildThread : public QRunnable
	{
	public:
		ChildThread(ThreadSharedData<Block> * sharedData, int index)
		{
			this->sharedData = sharedData;
			this->index = index;
		}

	protected:
		virtual void run();

	private:
		inline bool getNextCurrent(Block * outCurrent);
		inline void closeNodeGetG(const Block & block, double * outG);
		inline void scanArround(const Block & block, double blockG);

	private:
		ThreadSharedData<Block> * sharedData;
		int index;
	public:
		bool waiting = false;
		std::vector<Block> aroundBlock;
		std::vector<double> aroundG;
	};

	template<class Block>
	inline bool AStar::ChildThread<Block>::getNextCurrent(Block * outCurrent)
	{
		Block & current = *outCurrent;

		ThreadSharedData<Block> & d = *sharedData;

		while (!d.lockForOpen.tryLock());

		if (d.mapOpen.size() == 0)
		{
			d.lockForOpen.unlock();
			if (!waiting)
			{
				waiting = true;
				const int waitingCount = d.waitingThreadCount.fetchAndAddRelaxed(1) + 1;
				Q_ASSERT(waitingCount > 0 && waitingCount <= d.threadCount);
				// ȫ���̶߳�����ȴ����޷���������
				if (waitingCount == d.threadCount)
					d.finished = true;
			}

			return false;
		}

		if (waiting)
		{
			waiting = false;
			const int waitingCount = d.waitingThreadCount.fetchAndAddRelaxed(-1) - 1;
			Q_ASSERT(waitingCount >= 0 && waitingCount < d.threadCount);
		}

		// ��һ�����ƾ�����С��open�㣬���δ���
		std::multimap<double, Block>::iterator iterNext = d.mapOpen.begin();
		current = iterNext->second;

		d.mapOpen_reversed.remove(current);
		d.mapOpen.erase(iterNext);

		d.lockForOpen.unlock();

		return true;
	}


	template<class Block>
	void AStar::ChildThread<Block>::closeNodeGetG(const Block & block, double * outG)
	{
		ThreadSharedData<Block> & d = *sharedData;

		WinReadWriteLock * lockSingleCurrent = d.lockForSingle(block);
		while (!lockSingleCurrent->tryLockForWrite());

		AtomicHash<Block, Node<Block> >::iterator iterCurrent = d.mapAll.atomicFind(block);
		Q_ASSERT(iterCurrent != d.mapAll.end());
		Node<Block> * nodeCurrent = &iterCurrent.value();
		nodeCurrent->isClose = true;
		*outG = nodeCurrent->G;

		lockSingleCurrent->unlock();
	}


	template<class Block>
	void AStar::ChildThread<Block>::scanArround(const Block & block, double blockG)
	{
		ThreadSharedData<Block> & d = *sharedData;

		aroundBlock.clear();
		aroundG.clear();

		d.area->GetAroundBlock(block, aroundBlock, aroundG);

		for (int i = 0; i < (int)aroundBlock.size(); i++)
		{
			const Block & around = aroundBlock[i];

			if (d.area->GetBlockStatus(around) == BlockStatus_NoPass)
				continue;

			double thisG;
			if (i < (int)aroundG.size())
				thisG = blockG + aroundG[i];
			else
				thisG = blockG + d.area->FuncG(block, around);

			bool ignore = false;

			AtomicHash<Block, Node<Block> >::iterator iterAround = d.mapAll.atomicFind(around);
			bool exist = iterAround != d.mapAll.end();

			if (exist)
			{
				Node<Block> * nodeAround = &iterAround.value();
				WinReadWriteLock * lockForSingleAround = d.lockForSingle(around);
				while (!lockForSingleAround->tryLockForRead());
				// around�ڵ��Ѿ�close������open���Ǵ�block�ڵ㵽��around�ڵ㲢������̣�����
				if (nodeAround->isClose || thisG >= nodeAround->G)
					ignore = true;
				lockForSingleAround->unlock();
			}

			if (!ignore)
			{
				// �����ԣ�Ҫô��around�ڵ㻹������(!exist)��Ҫôexist&&open������ɨ�跢�ִ�block�߸���
				Node<Block> * nodeAround;

				WinReadWriteLock * lockSingleAround = d.lockForSingle(around);

				if (exist)
				{
					nodeAround = &iterAround.value();
					while (!lockSingleAround->tryLockForWrite());
				}
				else
				{
					// ����ɨ�跢�ֲ����ڣ�����������Ȼ�п��ܱ������߳����ȴ�����
					while (!lockSingleAround->tryLockForWrite());
					nodeAround = &d.mapAll.atomicCreate(around, Node<Block>(), false).value();
				}

				// ���ﻹҪ���һ��isClose����Ϊopen�Ļ��ټ����ж��޸�G������Ѿ�close��˵���������߳���ȥ��current��
				if (nodeAround->isClose)
					lockSingleAround->unlock();
				else
				{
					const bool isNew = (nodeAround->G == 0);
					if (isNew || thisG < nodeAround->G)
					{
						if (isNew)
							nodeAround->H = d.area->FuncH(around, d.end, d.begin);
						// ����¾���Ƚ�С�����޸ĸ��ڵ�
						nodeAround->parent = block;
						nodeAround->G = thisG;
					}
					lockSingleAround->unlock();

					while (!d.lockForOpen.tryLock());
					if (!isNew)
					{
						QHash<Block, ThreadSharedData<Block>::MultiMapIterator>::iterator iterOpenReversed = d.mapOpen_reversed.find(around);
						if (iterOpenReversed != d.mapOpen_reversed.end())
						{
							d.mapOpen.erase(iterOpenReversed.value());
							iterOpenReversed.value() = d.mapOpen.insert(std::make_pair(nodeAround->F(), around));
						}
						else
						{
							// ����lockForOpen.lockForWrite֮ǰ�����around�ڵ㱻�����߳���ȥ����current�ˣ��Ͳ�Ҫ�ټ���open�ˡ���
						}
					}
					else
					{
						std::multimap<double, Block>::iterator iterOpen = d.mapOpen.insert(std::make_pair(nodeAround->F(), around));
						Q_ASSERT(d.mapOpen_reversed.find(around) == d.mapOpen_reversed.end());
						d.mapOpen_reversed[around] = iterOpen;
					}
					d.lockForOpen.unlock();
				}
			}
		}
	}


	template<class Block>
	void AStar::ChildThread<Block>::run()
	{
		ThreadSharedData<Block> & d = *sharedData;

		while (true)
		{
			if(d.finished)
				break;

			Block current;

			if(!getNextCurrent(&current))
				continue;

			double currentG;
			closeNodeGetG(current, &currentG);
			
			if (current == d.end)
			{
				d.pathFound = true;
				d.finished = true;
				break;
			}

			scanArround(current, currentG);
		}
	}

}