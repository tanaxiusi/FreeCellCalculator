#include <QApplication>
#include <QTranslator>
#include <QTextCodec>
#include <QSettings>
#include <QtPlugin>
#include "CommonUI/MainDlg.h"

Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin)


int main(int argc, char *argv[])
{
	QApplication app(argc, argv);

	QString languageConfig;
	{
		QSettings setting(QCoreApplication::applicationDirPath() + "/FreeCellCalculator.ini", QSettings::IniFormat);
		languageConfig = setting.value("Other/Language", "Auto").toString();
	}

	if(languageConfig == "English" || languageConfig == "Auto" && QLocale::system().name() != "zh_CN")
	{
		QTranslator * translatorEnglish = new QTranslator(&app);
		translatorEnglish->load(":/Translation/en.qm");
		app.installTranslator(translatorEnglish);
	}

	
	MainDlg w;
	w.show();
	return app.exec();
}
