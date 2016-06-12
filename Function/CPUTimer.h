#pragma once
#include <QtGlobal>

class CPUTimer
{
public:
	static quint64 GetTickCount();
	static quint64 GetTickCountUS();

private:
	CPUTimer();
};