#pragma once

#include <QObject>
#include "SolutionThread.h"

class ThreadAbortHelper : public QObject
{
	Q_OBJECT

public:
	ThreadAbortHelper(SolutionThread * thr);
	~ThreadAbortHelper();

public slots:
	void requireAbort();

private:
	SolutionThread * m_thr;
	
};
