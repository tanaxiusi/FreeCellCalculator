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
	std::vector<MiniBoard> vecPath;
	AStar::GetPath(&ba, MiniBoard(m_game), MiniBoard(Board::GetFinishStatus()), vecPath, this);

	QList<Board> lstResult;
	for(int i = 0; i < (int)vecPath.size(); i++)
		lstResult << vecPath[i];

	if(!m_requireAbort)
		emit finish(lstResult);
	m_thread.quit();
}
