#pragma once
#include "Board.h"

class MiniBoard
{
private:
	qint8 data[52];
public:
	MiniBoard();
	MiniBoard(const Board & b);
	operator Board() const;

	bool operator < (const MiniBoard & other) const;
	bool operator == (const MiniBoard & other) const;
	bool operator != (const MiniBoard & other) const;
};

uint qHash(const MiniBoard & key);