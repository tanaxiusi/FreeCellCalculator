#pragma once

#include <QDialog>
#include "ui_ProgressDlg.h"
#include <QTimer>
#include <QTime>

class ProgressDlg : public QDialog
{
	Q_OBJECT

public:
	ProgressDlg(QWidget *parent = 0);
	~ProgressDlg();

protected:
	void closeEvent(QCloseEvent *);
	void reject();

public slots:
	void onProgressChange(bool isFinish, int openCount, int totalCount);

signals:
	void abort();
	

private:
	Ui::ProgressDlg ui;
};
