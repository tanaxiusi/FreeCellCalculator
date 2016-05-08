#pragma once

#include <QDialog>
#include "ui_ConfigDlg.h"

class ConfigDlg : public QDialog
{
	Q_OBJECT

public:
	ConfigDlg(QWidget *parent = 0);
	~ConfigDlg();

public slots:
	void onBtnOk();

private:
	Ui::ConfigDlg ui;
};
