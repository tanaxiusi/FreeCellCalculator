#include "PaintWidgetOpenGL.h"

PaintWidgetOpenGL::PaintWidgetOpenGL(QWidget *parent)
	: QGLWidget(parent)
{

}

PaintWidgetOpenGL::~PaintWidgetOpenGL()
{

}

void PaintWidgetOpenGL::paintEvent( QPaintEvent * )
{
	QPainter painter(this);
	emit requirePaint(&painter, this->width(), this->height());
}
