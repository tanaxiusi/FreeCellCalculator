#pragma once
#include <vector>
#include <map>
#include "AreaBase.h"
#include <QThreadPool>
#include "SubThread.h"

namespace AStar
{
	class ProgressReceiver
	{
	public:
		virtual void onProgressChange(bool isFinish, int openCount, int totalCount) = 0;
		virtual bool isAbort() = 0;
	};

	template<class Block>
	void GetPath(const AreaBase<Block> * pArea, const Block & begin, const Block & end, std::vector<Block> & outPath, ProgressReceiver * pReceiver = 0)
	{
		ThreadSharedData<Block> sharedData(pArea, begin, end, qMin(4, QThread::idealThreadCount()));

		auto & nodeBegin = sharedData.mapAll[begin];
		nodeBegin.parent = begin;
		sharedData.mapOpen_reversed[begin] = sharedData.mapOpen.insert(std::make_pair(nodeBegin.F(), begin));
		
		QThreadPool threadPool;
		threadPool.setMaxThreadCount(sharedData.threadCount);

		QList<SubThread<Block>* > m_lstThread;

		for (int i = 0; i < sharedData.threadCount; ++i)
		{
			SubThread<Block> * thr = new SubThread<Block>(&sharedData, i);
			m_lstThread << thr;
			threadPool.start(thr);
		}
		while(!threadPool.waitForDone(100))
			pReceiver->onProgressChange(false, 0, sharedData.mapAll.size());

		if(sharedData.pathFound && !(pReceiver && pReceiver->isAbort()))
		{
			outPath.insert(outPath.begin(), 1, end);

			Block pt = end;
			Block ptPrev = sharedData.mapAll[pt].parent;
			while(ptPrev != pt)
			{
				outPath.insert(outPath.begin(), 1, ptPrev);
				pt = ptPrev;
				ptPrev = sharedData.mapAll[pt].parent;
			}
		}
	}
}