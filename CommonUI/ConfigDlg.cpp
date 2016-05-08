#include "ConfigDlg.h"
#include <QSettings>
#include "Util/Other.h"

ConfigDlg::ConfigDlg(QWidget *parent)
	: QDialog(parent)
{
	ui.setupUi(this);
	connect(ui.btnOk, SIGNAL(clicked()), this, SLOT(onBtnOk()));
	connect(ui.btnCancel, SIGNAL(clicked()), this, SLOT(reject()));

	ui.comboBoxLanguage->addItems(tr(U8("跟随系统,中文,English")).split(","));
	ui.comboBoxLanguage->setItemData(0, "Auto");
	ui.comboBoxLanguage->setItemData(1, "Chinese");
	ui.comboBoxLanguage->setItemData(2, "English");

	QSettings setting(QCoreApplication::applicationDirPath() + "/FreeCellCalculator.ini", QSettings::IniFormat);
	ui.chkSmooth->setChecked(setting.value("Display/Smooth", 1).toInt() != 0);
	ui.chkOpenGL->setChecked(setting.value("Display/OpenGL", 1).toInt() != 0);
	ui.spinBox->setValue(setting.value("Display/AnimationDuration", 700).toInt());
	ui.comboBoxLanguage->setCurrentIndex(ui.comboBoxLanguage->findData(setting.value("Other/Language", "Auto").toString()));
}

ConfigDlg::~ConfigDlg()
{

}

void ConfigDlg::onBtnOk()
{
	QSettings setting(QCoreApplication::applicationDirPath() + "/FreeCellCalculator.ini", QSettings::IniFormat);
	setting.setValue("Display/Smooth", ui.chkSmooth->isChecked() ? 1 : 0);
	setting.setValue("Display/OpenGL", ui.chkOpenGL->isChecked() ? 1 : 0);
	setting.setValue("Display/AnimationDuration", ui.spinBox->value());
	setting.setValue("Other/Language", ui.comboBoxLanguage->itemData(ui.comboBoxLanguage->currentIndex()));

	emit accept();
}
