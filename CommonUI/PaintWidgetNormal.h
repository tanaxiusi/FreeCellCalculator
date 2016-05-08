#pragma once

#include <QWidget>

class PaintWidgetNormal : public QWidget
{
	Q_OBJECT

signals:
	void requirePaint(QPainter * pPainter, int width, int height);

public:
	PaintWidgetNormal(QWidget *parent);
	~PaintWidgetNormal();

protected:
	virtual void paintEvent(QPaintEvent *);
};

