#include "SolutionThread.h"
#include "BoardArea.h"
#include "AStar/Solution.h"
#include <QMetaType>

Q_DECLARE_METATYPE(QList<Board>);
int typeId = qRegisterMetaType<QList<Board> >("QList<Board>");

SolutionThread::SolutionThread(QObject *parent)
	: QObject(parent)
{
	m_requireAbort = false;
	connect(&m_thread, SIGNAL(started()), this, SLOT(onStart()));
	this->moveToThread(&m_thread);
	m_lastRefreshTime.start();
}

SolutionThread::~SolutionThread()
{
	m_thread.quit();
	m_thread.wait();
}

void SolutionThread::go( Board game )
{
	if(m_thread.isRunning())
		return;

	m_requireAbort = false;

	m_game = game;
	m_thread.start();
	
}

void SolutionThread::wait()
{
	m_thread.wait();
}


void SolutionThread::requireAbort()
{
	m_requireAbort = true;
}


void SolutionThread::onProgressChange( bool isFinish, int openCount, int totalCount )
{
	if(isFinish || m_lastRefreshTime.elapsed() > 100)
	{
		emit progressChange(isFinish, openCount, totalCount);
		m_lastRefreshTime.start();
	}
}

bool SolutionThread::isAbort()
{
	return m_requireAbort;
}

void SolutionThread::onStart()
{
	BoardArea ba;
	QList<MiniBoard> lstResult = AStar::getSolution(&ba, MiniBoard(m_game), MiniBoard(Board::GetFinishStatus()), this);

	QList<Board> lstConvertedResult;
	for (const MiniBoard & miniBoard : lstResult)
		lstConvertedResult << miniBoard;

	if(!m_requireAbort)
		emit finish(lstConvertedResult);
	m_thread.quit();
}
