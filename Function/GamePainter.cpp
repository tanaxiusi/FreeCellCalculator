#include "GamePainter.h"
#include "Util/Other.h"
#include <cmath>
#include "CPUTimer.h"

#define CARD_WIDTH 150
#define CARD_HEIGHT 200
// 列空白比例
#define columnBlankRatio 0.1
// 行空白比例
#define rowBlankRatio 0.2
// 卡牌纵向遮挡后露出的比例
#define shelterRatio 0.23

// 卡牌水平位移
static double columnPadding = (CARD_WIDTH * columnBlankRatio) / 2;
static double columnWidth = CARD_WIDTH * (columnBlankRatio + 1);

double Interpolator(double value, int level)
{
	double pow2 = pow(2, (double)(level - 1));
	if(value < 0)
	{
		return 0;
	}else if(value >= 0 && value < 0.5)
	{
		return pow2 * pow(value, level);
	}else if(value <= 1)
	{
		return 1 - pow2 * pow(1 - value, level);
	}else
	{
		return 1;
	}
}

GamePainter::GamePainter(QObject *parent)
	: QObject(parent)
{
	m_inAnimation = false;
	m_progress = 0;
	m_dynamicScaleEnable = false;
	m_maxProgress = 1000;
	m_smoothEnable = false;

	m_aboveRightIndex[0] = 2;
	m_aboveRightIndex[1] = 3;
	m_aboveRightIndex[2] = 1;
	m_aboveRightIndex[3] = 0;

	m_imgBk.load(":/Image/background.jpg");
	m_imgCard.load(":/Image/card.png");
	m_imgBorder.load(":/Image/border.png");
	m_imgBorder2.load(":/Image/border_below_fadeout.png");

	connect(&m_timer, SIGNAL(timeout()), this, SLOT(onTimer()));
	m_timer.setInterval(10);
}

GamePainter::~GamePainter()
{

}

void GamePainter::setDisplayData( const Board & board )
{
	m_data1 = board;
	m_inAnimation = false;
	m_timer.stop();
	this->requireRepaint();
}

void GamePainter::setDisplayData( const Board & board1, const Board & board2 )
{
	m_data1 = board1;
	m_data2 = board2;
	m_inAnimation = true;
	m_progress = 0;
	m_timer.start();
	m_time = CPUTimer::GetTickCount();
	this->requireRepaint();
}

void GamePainter::setAboveRightIndex(int club, int diamond, int heart, int spade)
{
	m_aboveRightIndex[0] = club;
	m_aboveRightIndex[1] = diamond;
	m_aboveRightIndex[2] = heart;
	m_aboveRightIndex[3] = spade;
}

void GamePainter::setDynamicScaleEnable( bool enable )
{
	m_dynamicScaleEnable = enable;
}

void GamePainter::setAnimationDuration( int duration )
{
	m_maxProgress = duration;
	if(m_maxProgress < 100)
		m_maxProgress = 100;
}

void GamePainter::setSmoothEnable( bool enable )
{
	m_smoothEnable = enable;
}


void GamePainter::onTimer()
{
	int lastProgress = m_progress;
	m_progress = (CPUTimer::GetTickCount() - m_time) % (m_maxProgress + 1);

	if(m_progress < lastProgress)
		emit playFinish();

	this->requireRepaint();
}

void GamePainter::onPaintEvent( QPainter * pPainter, int width, int height )
{
	QPainter & painter = *pPainter;
	// 移动中的卡牌，后面是始末位置
	QMap<Card,QPair<QRectF,QRectF> > mapMoving;
	// 前后两个状态下，mapMoving中卡牌的顺序
	QList<Card> lstSequenceBegin;
	QList<Card> lstSequenceEnd;

	double graphicProgress = (double)m_progress / m_maxProgress;
	// 对播放进度进行差值，使动画看起来更平滑
	graphicProgress = Interpolator(graphicProgress, 2);

	drawBackground(&painter, QSize(width, height));
	applyTransform(&painter, QSize(width, height), graphicProgress);

	if(m_smoothEnable)
		painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

	drawCardBorder(&painter);
	drawStaticCard(&painter);
	drawMovingCard(&painter, graphicProgress);
}

void GamePainter::applyTransform(QPainter * painter, QSize canvasSize, double graphicProgress)
{
	// 期望窗口尺寸
	double expectedWidth = CARD_WIDTH * (1 + columnBlankRatio) * 10;
	double scaleRatioX = (double)canvasSize.width() / expectedWidth;

	double scaleRatio = 0;

	double expectedHeight1 = 0;
	double expectedHeight2 = 0;
	// 计算下方最大牌数
	int maxCount1 = 0;
	int maxCount2 = 0;
	for(int column = 0; column < 8; column++)
	{
		int count1 = m_data1.GetBelowCount(column);
		if(count1 > maxCount1)
			maxCount1 = count1;

		if(!m_inAnimation)
			continue;

		int count2 = m_data2.GetBelowCount(column);
		if(count2 > maxCount2)
			maxCount2 = count2;
	}
	if(maxCount1 < 7)
		maxCount1 = 7;
	// 期望窗口高度为：顶部+中间+底部 3个行空白＋(maxCount-1)个遮挡露出高度＋2个卡牌尺寸
	expectedHeight1 = CARD_HEIGHT * (rowBlankRatio * 3 + shelterRatio * (maxCount1 - 1) + 2);

	if(maxCount2 < 7)
		maxCount2 = 7;
	expectedHeight2 = CARD_HEIGHT * (rowBlankRatio * 3 + shelterRatio * (maxCount2 - 1) + 2);

	// 计算2个状态的纵向缩放比
	double scaleRatioY1 = (double)canvasSize.height() / expectedHeight1;
	double scaleRatioY2 = (double)canvasSize.height() / expectedHeight2;

	// 计算2个状态的总缩放比
	double scaleRatio1 = qMin(scaleRatioX, scaleRatioY1);
	double scaleRatio2 = qMin(scaleRatioX, scaleRatioY2);

	if(m_inAnimation)
	{
		// 如果是播放动画
		if(m_dynamicScaleEnable)
		{
			// 开启动态缩放比，则根据进度线性渐变
			scaleRatio = scaleRatio1 * (1 - graphicProgress) + scaleRatio2 * graphicProgress;
		}else
		{
			// 不开启动态缩放比，取较小的比例
			scaleRatio = qMin(scaleRatio1, scaleRatio2);
		}
	}else
	{
		// 不播放动画，显示静态图像，就取第一个
		scaleRatio = scaleRatio1;
	}

	// 总体水平位移
	double globalPaddingLeft = (canvasSize.width() - expectedWidth * scaleRatio) / 2;

	QTransform transform;
	transform.translate(globalPaddingLeft, 0);
	transform.scale(scaleRatio, scaleRatio);
	painter->setTransform(transform);
}

void GamePainter::drawBackground(QPainter * painter, QSize canvasSize)
{
	painter->fillRect(0, 0, canvasSize.width(), canvasSize.height(), QColor(128,128,128));
	painter->drawPixmap(QRect(0,0,canvasSize.width(),canvasSize.height()), m_imgBk);
}

void GamePainter::drawCardBorder(QPainter * painter)
{
	// 左上
	for(int index = 0; index < 4; index++)
		painter->drawPixmap(getAboveLeftRect(index), m_imgBorder, getPixmapFullRect(m_imgBorder));

	// 右上
	for(int index = 0; index < 4; index++)
		painter->drawPixmap(getAboveRightRect(convertAboveRightIndex((Card::Shape)index)), m_imgBorder, getPixmapFullRect(m_imgBorder));

	// 下
	for(int index = 0; index < 8; index++)
		painter->drawPixmap(getBelowRect(index, 0), m_imgBorder2, getPixmapFullRect(m_imgBorder2));
}

void GamePainter::drawStaticCard(QPainter * painter)
{
	// 左上
	for(int index = 0; index < 4; index++)
	{
		Card card = m_data1.aboveLeft[index];
		// 如果开启动画播放，且状态2此处与状态1不一样，就说明状态1的这张卡牌正在移动，不能画。下同
		if(m_inAnimation && m_data2.aboveLeft[index] != card)
			continue;
		if(card.isValid())
			painter->drawPixmap(getAboveLeftRect(index), m_imgCard, getCardSourceRect(card));
	}

	// 右上
	for(int index = 0; index < 4; index++)
	{
		Card card = m_data1.aboveRight[index];
		if(card.isValid())
			painter->drawPixmap(getAboveRightRect(convertAboveRightIndex((Card::Shape)index)), m_imgCard, getCardSourceRect(card));
	}

	// 下
	for(int column = 0; column < 8; column++)
	for(int index = 0; index < MaxCardCount; index++)
	{
		Card card = m_data1.below[column][index];
		if(m_inAnimation && m_data2.below[column][index] != card)
			continue;
		if(card.isValid())
			painter->drawPixmap(getBelowRect(column, index), m_imgCard, getCardSourceRect(card));
		else
			break;
	}
}

void GamePainter::drawMovingCard( QPainter * painter, double graphicProgress )
{
	if(!m_inAnimation)
		return;

	QList<Card> cardOrderInBegin;
	QList<Card> cardOrderInEnd;
	QMap<Card, QPair<QRectF,QRectF> > movingCardPosition;

	// 首先扫描始末两个状态，计算出不同的卡牌和它们的顺序

	// 左上
	for(int index = 0; index < 4; index++)
	{
		Card card = m_data1.aboveLeft[index];
		Card card2 = m_data2.aboveLeft[index];
		if(card == card2)
			continue;

		if(card.isValid())
		{
			// 如果状态1该位置的卡牌是有效的（有卡牌），就把这个位置设为该卡牌的起始位置
			movingCardPosition[card].first = getAboveLeftRect(index);
			if(!cardOrderInBegin.contains(card))
				cardOrderInBegin << card;
		}
		if(card2.isValid())
		{
			// 如果状态2该位置的卡牌是有效的（有卡牌），就把这个位置设为该卡牌的结束位置
			movingCardPosition[card2].second = getAboveLeftRect(index);
			if(!cardOrderInEnd.contains(card2))
				cardOrderInEnd << card2;
		}
	}
	// 下方
	for(int column = 0; column < 8; column++)
	for(int index = 0; index < MaxCardCount; index++)
	{
		Card card = m_data1.below[column][index];
		Card card2 = m_data2.below[column][index];
		if(card == card2)
			continue;

		if(card.isValid())
		{
			movingCardPosition[card].first = getBelowRect(column, index);
			if(!cardOrderInBegin.contains(card))
				cardOrderInBegin << card;
		}
		if(card2.isValid())
		{
			movingCardPosition[card2].second = getBelowRect(column, index);
			if(!cardOrderInEnd.contains(card2))
				cardOrderInEnd << card2;
		}

		if(!card.isValid() && !card2.isValid())
			break;
	}

	// cardThis按遮挡顺序存储“主”状态中跟另外状态不一样的卡牌，cardOther存储“次”状态的
	QList<Card> cardThis;
	QList<Card> cardOther;

	// 动画播放的前半段，以状态1为主状态，动画后半段以2为主状态。主状态的含义是使用该状态的遮挡顺序
	if(graphicProgress < 0.5)
	{
		cardThis = cardOrderInBegin;
		cardOther = cardOrderInEnd;
	}else
	{
		cardThis = cardOrderInEnd;
		cardOther = cardOrderInBegin;
	}

	QList<Card> cardRecycled;

	// 如果次状态移动中的卡牌在主状态找不到，就说明它被移到右上角回收区了
	foreach(Card card, cardOther)
		if(!cardThis.contains(card))
			cardRecycled << card;

	// 对缺失（移到回收区）的卡牌仅按大小排序，以确保动画后半段卡牌即将移到右上角回收区时遮挡顺序正确
	// 该排序操作最大程度保持原不同花色卡牌间的顺序
	cardRecycled = cardSort(cardRecycled);

	// 移到回收区的卡牌压在底下
	cardThis = cardRecycled + cardThis;

	foreach(Card card, cardThis)
	{
		// 取出该卡牌移动的始末位置
		QRectF rcBegin = movingCardPosition[card].first;
		QRectF rcEnd = movingCardPosition[card].second;

		// 如果始/末位置为空，则指向回收区位置
		if(rcBegin == QRectF())
			rcBegin = getAboveRightRect(convertAboveRightIndex(card.shape()));
		if(rcEnd == QRectF())
			rcEnd = getAboveRightRect(convertAboveRightIndex(card.shape()));

		// 目标位置采用线性差值
		QRectF rcDest;
		rcDest.setLeft(rcBegin.left() * (1 - graphicProgress) + rcEnd.left() * graphicProgress);
		rcDest.setTop(rcBegin.top() * (1 - graphicProgress) + rcEnd.top() * graphicProgress);
		rcDest.setRight(rcBegin.right() * (1 - graphicProgress) + rcEnd.right() * graphicProgress);
		rcDest.setBottom(rcBegin.bottom() * (1 - graphicProgress) + rcEnd.bottom() * graphicProgress);

		painter->drawPixmap(rcDest, m_imgCard, getCardSourceRect(card));
	}
}

int GamePainter::convertAboveRightIndex(Card::Shape shape)
{
	if(shape < 0)
		shape = (Card::Shape)0;
	if(shape > 3)
		shape = (Card::Shape)3;
	return m_aboveRightIndex[shape];
}


QRectF GamePainter::getCardSourceRect(Card card)
{
	return QRectF((int)card.shape() * CARD_WIDTH, (card.number() - 1) * CARD_HEIGHT, CARD_WIDTH, CARD_HEIGHT);
}

QRectF GamePainter::getPixmapFullRect(QPixmap pixmap)
{
	return QRectF(QPointF(0, 0), pixmap.size());
}

QRectF GamePainter::getAboveLeftRect(int index)
{
	return QRectF(columnWidth * (index + 0.5) + columnPadding, CARD_HEIGHT * rowBlankRatio, CARD_WIDTH, CARD_HEIGHT);
}

QRectF GamePainter::getAboveRightRect(int index)
{
	return QRectF(columnWidth * (index + 5.5) + columnPadding, CARD_HEIGHT * rowBlankRatio, CARD_WIDTH, CARD_HEIGHT);
}

QRectF GamePainter::getBelowRect(int column, int index)
{
	return QRectF(columnWidth * (column + 1) + columnPadding, CARD_HEIGHT * (shelterRatio * index + 1 + rowBlankRatio * 2), CARD_WIDTH, CARD_HEIGHT);
}

QList<Card> GamePainter::cardSort( QList<Card> cardList )
{
	QList<Card> lstTemp;
	foreach(Card card, cardList)
	{
		Card::Shape shape = card.shape();
		qint8 number = card.number();

		int currentIndex = lstTemp.size();
		for(int i = lstTemp.size() - 1; i >= 0; i--)
		{
			Card card2 = lstTemp[i];
			Card::Shape shape2 = card2.shape();
			qint8 number2 = card2.number();

			// 如果是同花色，就进行排序
			if(shape == shape2 && number < number2)
				currentIndex = i;
			else if(shape == shape2 && number > number2)
				break;
		}
		lstTemp.insert(currentIndex, card);
	}
	return lstTemp;

}
