#include "ProgressDlg.h"
#include <QCloseEvent>
#include "Util/Other.h"

ProgressDlg::ProgressDlg(QWidget *parent)
	: QDialog(parent)
{
	ui.setupUi(this);
}

ProgressDlg::~ProgressDlg()
{

}


void ProgressDlg::closeEvent( QCloseEvent * e)
{
	e->ignore();
	emit abort();
}


void ProgressDlg::reject()
{
	close();
}


void ProgressDlg::onProgressChange( bool isFinish, int openCount, int totalCount )
{
	Q_UNUSED(openCount);
	ui.label->setText(tr(U8("已搜索状态数 %1")).arg(totalCount));
	if(isFinish)
		accept();
}