#pragma once
#include "AreaBase.h"
#include <QThreadPool>
#include "ChildThread.h"

namespace AStar
{
	class ProgressReceiver
	{
	public:
		virtual void onProgressChange(bool isFinish, int openCount, int totalCount) = 0;
		virtual bool isAbort() = 0;
	};

	template<class Block>
	QList<Block> getSolution(const AreaBase<Block> * pArea, const Block & begin, const Block & end, ProgressReceiver * pReceiver = NULL)
	{
		ThreadSharedData<Block> sharedData(pArea, begin, end, qMin(4, QThread::idealThreadCount()));

		Node<Block> & nodeBegin = sharedData.mapAll[begin];
		nodeBegin.parent = begin;
		sharedData.mapOpen_reversed[begin] = sharedData.mapOpen.insert(std::make_pair(nodeBegin.F(), begin));
		
		QThreadPool threadPool;
		threadPool.setMaxThreadCount(sharedData.threadCount);

		for (int i = 0; i < sharedData.threadCount; ++i)
			threadPool.start(new ChildThread<Block>(&sharedData, i));

		while (!threadPool.waitForDone(50))
		{
			pReceiver->onProgressChange(false, 0, sharedData.mapAll.size());
			if (pReceiver && pReceiver->isAbort())
				sharedData.finished = true;
		}

		QList<Block> lstResult;

		if(sharedData.pathFound && !(pReceiver && pReceiver->isAbort()))
		{
			lstResult.insert(0, end);

			Block pt = end;
			Block ptPrev = sharedData.mapAll[pt].parent;
			while(ptPrev != pt)
			{
				lstResult.insert(0, ptPrev);
				pt = ptPrev;
				ptPrev = sharedData.mapAll[pt].parent;
			}
		}

		if(pReceiver)
			pReceiver->onProgressChange(true, sharedData.mapOpen.size(), sharedData.mapAll.size());

		return lstResult;
	}
}