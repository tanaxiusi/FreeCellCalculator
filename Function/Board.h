#pragma once
#include "Card.h"

#define MaxCardCount 20

struct Board 
{
public:
	Card below[8][MaxCardCount];
	Card aboveLeft[4];
	Card aboveRight[4];
public:
	static Board GetFinishStatus();
public:
	Board();
	bool operator == (const Board & other) const;
	bool operator != (const Board & other) const;
	bool operator < (const Board & other) const;

	int GetBelowCount(int column) const;
	int GetFinishCount() const;

	Card GetBelow(int column, int index = 0) const;
	Card GetAboveLeft(int index) const;
	Card GetAboveRight(Card::Shape shape) const;

	Card PopAboveLeft(int index);
	Card PopBelow(int column);

	bool PushAboveLeft(Card card);
	bool PushAboveRight(Card card);
	bool PushBelow(int column, Card card);
	void AutoFinish();

	bool IsValid() const;
	
public:
	int GetBelowContinuousCount(int column) const;

private:
	static bool AutoFinishStepByStep;
};
