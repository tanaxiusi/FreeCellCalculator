#ifndef GAMEPAINTER_H
#define GAMEPAINTER_H

#include <QObject>
#include "Board.h"
#include <QPainter>
#include <QTimer>
#include <QTime>
#include <QMap>

class GamePainter : public QObject
{
	Q_OBJECT

signals:
	void playFinish();
	void requireRepaint();

public:
	GamePainter(QObject *parent = NULL);
	~GamePainter();

public:
	void setDisplayData(const Board & board);
	void setDisplayData(const Board & board, const Board & board2);
	void setAboveRightIndex(int club, int diamond, int heart, int spade);
	void setDynamicScaleEnable(bool enable);

	void setAnimationDuration(int duration);
	void setSmoothEnable(bool enable);

private slots:
	void onTimer();
	void onPaintEvent(QPainter * pPainter, int width, int height);

private:
	void applyTransform(QPainter * painter, QSize canvasSize, double graphicProgress);
	void drawBackground(QPainter * painter, QSize canvasSize);
	void drawCardBorder(QPainter * painter);
	void drawStaticCard(QPainter * painter);
	void drawMovingCard(QPainter * painter, double graphicProgress);

	int convertAboveRightIndex(Card::Shape shape);

private:
	static QRectF getCardSourceRect(Card card);
	static QRectF getPixmapFullRect(QPixmap pixmap);
	static QRectF getAboveLeftRect(int index);
	static QRectF getAboveRightRect(int index);
	static QRectF getBelowRect(int column, int index);
	static QList<Card> cardSort(QList<Card> cardList);

private:
	QPixmap m_imgBk;
	QPixmap m_imgCard;
	QPixmap m_imgBorder;
	QPixmap m_imgBorder2;

	Board m_data1;
	Board m_data2;
	bool m_inAnimation;
	QTimer m_timer;
	QTime m_time;
	int m_progress;
	int m_aboveRightIndex[4];
	bool m_dynamicScaleEnable;
	int m_maxProgress;
	bool m_smoothEnable;
	
	
};

#endif // GAMEPAINTER_H
