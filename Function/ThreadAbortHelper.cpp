#include "ThreadAbortHelper.h"

ThreadAbortHelper::ThreadAbortHelper(SolutionThread * thr)
	: QObject(NULL)
{
	this->m_thr = thr;

}

ThreadAbortHelper::~ThreadAbortHelper()
{

}

void ThreadAbortHelper::requireAbort()
{
	if(m_thr)
		m_thr->requireAbort();
}
