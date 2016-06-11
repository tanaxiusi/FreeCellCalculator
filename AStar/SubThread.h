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
						// ȫ���̶߳�����ȴ����޷���������
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

				// ��һ�����ƾ�����С��open�㣬���δ���
				std::multimap<double, Block>::iterator iterNext = d.mapOpen.begin();
				current = iterNext->second;

				d.mapOpen_reversed.erase(current);
				d.mapOpen.erase(iterNext);

				d.lockForOpen.unlock();
			}

			// node��Ϊclose
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
					// around�ڵ��Ѿ�close������open���Ǵ�current�ڵ㵽��around�ڵ㲢������̣�����
					if (nodeAround->isClose || thisG >= nodeAround->G)
						ignore = true;
					lockForSingleAround->unlock();
				}

				if (!ignore)
				{
					// �����ԣ�Ҫô��around�ڵ㻹������(!exist)��Ҫôexist&&open������ɨ�跢�ִ�current�߸���
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
						// ����ɨ�跢�ֲ����ڣ�����������Ȼ�п��ܱ������߳����ȴ�����
						while(!lockSingleAround->tryLockForWrite())
							try8++;
						nodeAround = &d.mapAll.atomicCreate(around, Node<Block>(), false).value();
					}

					// ���ﻹҪ���һ��isClose����Ϊopen�Ļ��ټ����ж��޸�G������Ѿ�close��˵���������߳���ȥ��current��
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
							// ����¾���Ƚ�С�����޸ĸ��ڵ�
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
								// ����lockForOpen.lockForWrite֮ǰ�����around�ڵ㱻�����߳���ȥ����current�ˣ��Ͳ�Ҫ�ټ���open�ˡ���
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