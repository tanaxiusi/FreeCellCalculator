#pragma once

#include <QObject>
#include <QThread>
#include <QTime>
#include "AStar/Solution.h"
#include "Board.h"

class SolutionThread : public QObject, public AStar::ProgressReceiver
{
	Q_OBJECT

public:
	SolutionThread(QObject *parent = NULL);
	~SolutionThread();

	void go(Board game);
	void wait();
	void requireAbort();
	virtual void onProgressChange(bool isFinish, int openCount, int totalCount);
	virtual bool isAbort();

signals:
	void finish(QList<Board> result);
	void progressChange(bool isFinish, int openCount, int totalCount);

public slots:
	void onStart();

private:
	QThread m_thread;
	QTime m_lastRefreshTime;
	Board m_game;
	bool m_requireAbort;
};
