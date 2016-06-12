#include "CPUTimer.h"
#include <Windows.h>

static LARGE_INTEGER frequency;

static int initFunc()
{
	QueryPerformanceFrequency(&frequency);
	return 0;
}

static int dummy = initFunc();

quint64 CPUTimer::GetTickCount()
{
	LARGE_INTEGER counter = {0};
	QueryPerformanceCounter(&counter);
	return counter.QuadPart * 1000 / frequency.QuadPart;
}

quint64 CPUTimer::GetTickCountUS()
{
	LARGE_INTEGER counter = {0};
	QueryPerformanceCounter(&counter);
	return counter.QuadPart * 1000000 / frequency.QuadPart;
}

