#pragma once

#include <QMainWindow>
#include "ui_MainDlg.h"
#include "Function/Board.h"
#include "Function/GamePainter.h"

class MainDlg : public QMainWindow
{
	Q_OBJECT

public:
	MainDlg(QWidget *parent = 0);
	~MainDlg();

public slots:
	void onActionNewGame();
	void onActionCalculate();
	void onActionPrev();
	void onActionNext();
	void onActionPlay(bool checked);
	void onActionConfig();
	void onCalculationFinish(QList<Board> result);
	void onWidgetPlayFinish();

private:
	void refreshDisplay();
	void loadSetting();
	void recreateDisplay();
	QList<int> getSolutionAboveRightIndex();

private:
	Ui::MainDlgClass ui;
	Board m_game;
	QList<Board> m_solution;
	int m_solutionIndex;
	bool m_playing;
	GamePainter m_gamePainter;
	bool m_useOpenGL;
	QWidget * m_displayWidget;
};
