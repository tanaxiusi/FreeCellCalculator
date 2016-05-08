#include "PaintWidgetNormal.h"
#include <QPainter>

PaintWidgetNormal::PaintWidgetNormal(QWidget *parent)
	: QWidget(parent)
{

}

PaintWidgetNormal::~PaintWidgetNormal()
{

}

void PaintWidgetNormal::paintEvent( QPaintEvent * )
{
	QPainter painter(this);
	emit requirePaint(&painter, this->width(), this->height());
}
