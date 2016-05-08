#include "Card.h"

const Card Card::Invalid = Card::fromNativeIndex(0);

Card::Card(qint8 number, Shape shape)
{
	data = (number - 1) * 4 + shape + 1;
	if(!(data >= 1 && data <= 52))
		data = 0;
}

Card::Card()
{
	data = 0;
}

Card Card::fromNativeIndex(char data)
{
	Card ret(1, Club);
	ret.data = data;
	return ret;
}

Card::Color Card::color() const
{
	const Shape myShape = shape();
	return (myShape == Diamond || myShape == Heart) ? Red :Black;
}

Card::Color Card::color(Shape shape)
{
	return (shape == Diamond || shape == Heart) ? Red :Black;
}

Card::Shape Card::shape() const
{
	return (Shape)((data - 1) % 4);
}

qint8 Card::number() const
{
	return isValid() ? ((data - 1) / 4 + 1) : 0;
}

bool Card::isValid() const
{
	return data;
}

qint8 Card::nativeIndex() const
{
	return data;
}

bool Card::operator==(const Card & other) const
{
	return this->data == other.data;
}

bool Card::operator!=(const Card & other) const
{
	return this->data != other.data;
}

bool Card::operator<(const Card & other) const
{
	return this->data < other.data;
}
