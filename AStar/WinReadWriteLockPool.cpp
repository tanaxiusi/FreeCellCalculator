#include "WinReadWriteLockPool.h"



WinReadWriteLockPool::WinReadWriteLockPool(int size)
	:mutexes(size)
{
	for (int index = 0; index < mutexes.count(); ++index) {
		mutexes[index].store(0);
	}
}

WinReadWriteLockPool::~WinReadWriteLockPool()
{
	for (int index = 0; index < mutexes.count(); ++index)
		delete mutexes[index].load();
}

WinReadWriteLock * WinReadWriteLockPool::createMutex(int index)
{
	// mutex not created, create one
	WinReadWriteLock *newMutex = new WinReadWriteLock();
	if (!mutexes[index].testAndSetRelease(0, newMutex))
		delete newMutex;
	return mutexes[index].load();
}

