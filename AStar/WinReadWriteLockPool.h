#pragma once
#include <QVarLengthArray>
#include "WinReadWriteLock.h"

class WinReadWriteLockPool
{
public:
	explicit WinReadWriteLockPool(int size = 1697);
	~WinReadWriteLockPool();

	inline WinReadWriteLock *get(uint h) {
		int index = h % mutexes.count();
		WinReadWriteLock *m = mutexes[index].load();
		if (m)
			return m;
		else
			return createMutex(index);
	}

private:
	WinReadWriteLock *createMutex(int index);
	QVarLengthArray<QAtomicPointer<WinReadWriteLock>, 1697> mutexes;
};