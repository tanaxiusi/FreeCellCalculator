#pragma once
#include <QtGlobal>

class Card
{
public:
	enum Color{Black = 0, Red = 1};
	enum Shape{Club = 0, Diamond = 1, Heart = 2, Spade = 3};

public:
	Card();
	Card(qint8 number, Shape shape);
	static Card fromNativeIndex(char data);

	Color color() const;
	static Color color(Shape shape);

	Shape shape() const;
	qint8 number() const;

	bool isValid() const;
	qint8 nativeIndex() const;

	bool operator== (const Card & other) const;
	bool operator!= (const Card & other) const;
	bool operator < (const Card & other) const;

public:
	static const Card Invalid;

private:
	qint8 data;
};
