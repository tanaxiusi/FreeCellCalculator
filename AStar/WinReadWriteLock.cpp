#include "WinReadWriteLock.h"

WinReadWriteLock::WinReadWriteLock()
{
	InitializeSRWLock(&m_lock);
}


WinReadWriteLock::~WinReadWriteLock()
{

}

void WinReadWriteLock::lockForRead()
{
	AcquireSRWLockShared(&m_lock);
}

bool WinReadWriteLock::tryLockForRead()
{
	return TryAcquireSRWLockShared(&m_lock);
}

void WinReadWriteLock::lockForWrite()
{
	AcquireSRWLockExclusive(&m_lock);
}

bool WinReadWriteLock::tryLockForWrite()
{
	return TryAcquireSRWLockExclusive(&m_lock);
}

void WinReadWriteLock::unlock()
{
	if (m_lock.Ptr == (PVOID)1)
		unlockForWrite();
	else
		unlockForRead();
}

void WinReadWriteLock::unlockForRead()
{
	ReleaseSRWLockShared(&m_lock);
}

void WinReadWriteLock::unlockForWrite()
{
	ReleaseSRWLockExclusive(&m_lock);
}
