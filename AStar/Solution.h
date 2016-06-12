#pragma once
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

		for (int i = 0; i < sharedData.threadCount; ++i)
			threadPool.start(new SubThread<Block>(&sharedData, i));

		while (!threadPool.waitForDone(50))
		{
			pReceiver->onProgressChange(false, 0, sharedData.mapAll.size());
			if (pReceiver && pReceiver->isAbort())
				sharedData.finished = true;
		}

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

		if(pReceiver)
			pReceiver->onProgressChange(true, sharedData.mapOpen.size(), sharedData.mapAll.size());
	}
}