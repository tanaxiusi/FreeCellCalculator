#include <QApplication>
#include <QTranslator>
#include <QTextCodec>
#include <QSettings>
#include "CommonUI/MainDlg.h"

int main(int argc, char *argv[])
{
	QApplication app(argc, argv);
	app.addLibraryPath(QApplication::applicationDirPath() + "/plugins");

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
