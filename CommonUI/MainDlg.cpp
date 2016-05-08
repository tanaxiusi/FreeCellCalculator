#include "MainDlg.h"
#include <QMessageBox>
#include <QSettings>
#include <QPainter>
#include <QInputDialog>
#include "Util/Other.h"
#include "Function/SolutionThread.h"
#include "Function/ThreadAbortHelper.h"
#include "Function/BoardArea.h"
#include "Function/Shuffle.h"
#include "AStar/Solution.h"
#include "PaintWidgetNormal.h"
#include "PaintWidgetOpenGL.h"
#include "ConfigDlg.h"
#include "ProgressDlg.h"

MainDlg::MainDlg(QWidget *parent, Qt::WFlags flags)
	: QMainWindow(parent, flags)
{
	m_solutionIndex = 0;
	m_playing = false;
	m_displayWidget = NULL;

	ui.setupUi(this);

	connect(ui.actionNewgame, SIGNAL(triggered()), this, SLOT(onActionNewGame()));
	connect(ui.actionCalculate, SIGNAL(triggered()), this, SLOT(onActionCalculate()));
	connect(ui.actionPrev, SIGNAL(triggered()), this, SLOT(onActionPrev()));
	connect(ui.actionNext, SIGNAL(triggered()), this, SLOT(onActionNext()));
	connect(ui.actionPlay, SIGNAL(triggered(bool)), this, SLOT(onActionPlay(bool)));
	connect(ui.actionConfig, SIGNAL(triggered()), this, SLOT(onActionConfig()));
	connect(&m_gamePainter, SIGNAL(playFinish()), this, SLOT(onWidgetPlayFinish()));

	loadSetting();
}

MainDlg::~MainDlg()
{

}

void MainDlg::onActionNewGame()
{
	bool ok = false;
	qsrand(QDateTime::currentMSecsSinceEpoch());
	const int gameIndex = QInputDialog::getInt(this, tr(U8("选择游戏")), tr(U8("输入游戏编号 1-1000000")), qrand(), -4, 1000000, 1, &ok);
	
	if(!ok)
		return;
	
	if(gameIndex >= -4 && gameIndex <= -1)
	{
		m_game = ShuffleSpecial(gameIndex);
	}else if(gameIndex >= 1 && gameIndex <= 1000000)
	{
		m_game = Shuffle(gameIndex);		
	}else
	{
		QMessageBox::warning(this, tr(U8("选择游戏")), tr(U8("%1 不是有效的游戏编号")).arg(gameIndex));
		return;
	}

	m_solution.clear();
	m_solutionIndex = 0;
	ui.actionPlay->setChecked(false);
	onActionPlay(false);

	ui.statusBar->showMessage(tr(U8("游戏 #%1")).arg(gameIndex));
	m_gamePainter.setDisplayData(m_game);
}

void MainDlg::onActionCalculate()
{
	if(m_solution.size() > 0)
		return;

	ProgressDlg dlg;
	SolutionThread thr;
	ThreadAbortHelper helper(&thr);
	connect(&thr, SIGNAL(progressChange(bool,int,int)), &dlg, SLOT(onProgressChange(bool,int,int)));
	connect(&dlg, SIGNAL(abort()), &helper, SLOT(requireAbort()));
	connect(&thr, SIGNAL(finish(QList<Board>)), this, SLOT(onCalculationFinish(QList<Board>)));
	thr.go(m_game);
	dlg.exec();

	thr.wait();
	disconnect(&thr, SIGNAL(finish(QList<Board>)), this, SLOT(onCalculationFinish(QList<Board>)));
}

void MainDlg::onActionPrev()
{
	if(m_solution.size() == 0)
		return;

	m_solutionIndex--;
	if(m_solutionIndex < 0)
		m_solutionIndex = 0;

	refreshDisplay();
}

void MainDlg::onActionNext()
{
	if(m_solution.size() == 0)
		return;

	m_solutionIndex++;
	if(m_solutionIndex > m_solution.size() - 1)
		m_solutionIndex = m_solution.size() - 1;

	refreshDisplay();
}

void MainDlg::onActionPlay( bool checked )
{
	m_playing = checked;
	ui.actionPrev->setEnabled(!checked);
	ui.actionNext->setEnabled(!checked);
	m_gamePainter.setDynamicScaleEnable(checked);
	if(checked && m_solutionIndex == m_solution.size() - 1)
	{
		// 在最后一步时点击播放，从头开始
		m_solutionIndex = 0;
		refreshDisplay();
	}
}

void MainDlg::onActionConfig()
{
	ConfigDlg dlg;
	if(dlg.exec() == QDialog::Accepted)
		loadSetting();
}


void MainDlg::onCalculationFinish( QList<Board> result )
{
	m_solution = result;
	m_solutionIndex = 0;

	QList<int> aboveRightIndex = getSolutionAboveRightIndex();
	m_gamePainter.setAboveRightIndex(aboveRightIndex[0], aboveRightIndex[1], aboveRightIndex[2], aboveRightIndex[3]);
	
	if(m_solution.size() == 0)
		QMessageBox::warning(this, tr(U8("计算")), tr(U8("此局无解")));

	refreshDisplay();
}


void MainDlg::onWidgetPlayFinish()
{
	// 单步动画播放完毕
	if(!m_playing)
		return;

	// 下一步
	m_solutionIndex++;
	if(m_solutionIndex == m_solution.size() - 1)
	{
		// 到最后了，把按钮弹起
		ui.actionPlay->setChecked(false);
		onActionPlay(false);
	}else if(m_solutionIndex > m_solution.size() - 1)
		m_solutionIndex = 0;
	
	refreshDisplay();
}



void MainDlg::refreshDisplay()
{
	if(m_solutionIndex >= 0 && m_solutionIndex < m_solution.size() - 1)
		m_gamePainter.setDisplayData(m_solution[m_solutionIndex], m_solution[m_solutionIndex + 1]);
	else if(m_solutionIndex == m_solution.size() - 1)
		m_gamePainter.setDisplayData(m_solution[m_solutionIndex]);
}

void MainDlg::loadSetting()
{
	QSettings setting(QCoreApplication::applicationDirPath() + "/FreeCellCalculator.ini", QSettings::IniFormat);
	m_gamePainter.setSmoothEnable(setting.value("Display/Smooth", 1).toInt() != 0);
	m_gamePainter.setAnimationDuration(setting.value("Display/AnimationDuration", 700).toInt());
	const bool originalUseOpenGL = m_useOpenGL;
	m_useOpenGL = setting.value("Display/OpenGL", 1).toInt() != 0;
	if(m_displayWidget == NULL || originalUseOpenGL != m_useOpenGL)
		recreateDisplay();

}

void MainDlg::recreateDisplay()
{
	if(m_displayWidget)
	{
		ui.horizontalLayout->removeWidget(m_displayWidget);
		delete m_displayWidget;
		m_displayWidget = NULL;
	}
	if(m_useOpenGL)
		m_displayWidget = new PaintWidgetOpenGL(ui.centralWidget);
	else
		m_displayWidget = new PaintWidgetNormal(ui.centralWidget);

	ui.horizontalLayout->addWidget(m_displayWidget);
	connect(m_displayWidget, SIGNAL(requirePaint(QPainter*,int,int)), &m_gamePainter, SLOT(onPaintEvent(QPainter*,int,int)));
	connect(&m_gamePainter, SIGNAL(requireRepaint()), m_displayWidget, SLOT(repaint()));
}

QList<int> MainDlg::getSolutionAboveRightIndex()
{
	QList<int> aboveRightIndex;
	aboveRightIndex << 0 << 0 << 0 << 0;
	int finishCount = 0;

	// 扫描全部状态，算出右上回收区四种花色的先后顺序
	for(int i = 0; i < (int)m_solution.size() - 1; i++)
	{
		const Board & current = m_solution[i];
		const Board & next = m_solution[i + 1];
		QList<Card::Shape> lstShapeThisFinish;
		for(char shapeIndex = 0; shapeIndex < 4; shapeIndex++)
		{
			const Card::Shape shape = (Card::Shape)shapeIndex;
			if(current.GetAboveRight(shape).isValid() == false && next.GetAboveRight(shape).isValid())
				lstShapeThisFinish << shape;
		}


		if(lstShapeThisFinish.size() == 1)
		{
			// 当这一步只有1种花色的A被回收时，该花色就是下一个
			aboveRightIndex[lstShapeThisFinish.first()] = finishCount++;
			if(finishCount > 4)
				return aboveRightIndex;

		}else if(lstShapeThisFinish.size() > 1)
		{
			// 当这一步有多种花色的A被回收时
			for(int j = 0; j < 8; j++)
			{
				// 需要扫描所有列
				int len = current.GetBelowCount(j);
				for(int k = 0; k < len; k++)
				{
					// 的所有牌
					foreach(Card::Shape shape, lstShapeThisFinish)
					{
						// 越下面，就越先
						if(Card(1, shape) == current.GetBelow(j, k))
						{
							aboveRightIndex[shape] = finishCount++;
							if(finishCount > 4)
								return aboveRightIndex;
						}
					}
				}
			}

		}
	}

	if(finishCount < 4)
	{
		for(int i = 0; i < 4; i++)
			if(aboveRightIndex[i] == 0)
				aboveRightIndex[i] = finishCount++;
	}

	return aboveRightIndex;
}
