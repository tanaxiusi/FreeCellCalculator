#pragma once

#include <QtOpenGL/QGLWidget>

class PaintWidgetOpenGL : public QGLWidget
{
	Q_OBJECT

signals:
	void requirePaint(QPainter * pPainter, int width, int height);

public:
	PaintWidgetOpenGL(QWidget *parent);
	~PaintWidgetOpenGL();

protected:
	virtual void paintEvent(QPaintEvent *);
	
};

