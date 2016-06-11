#pragma once

#include <Windows.h>
#include <QtGlobal>

class WinReadWriteLock
{
public:
	WinReadWriteLock();
	~WinReadWriteLock();

	void lockForRead();
	bool tryLockForRead();
	bool tryLockForRead(int timeout);

	void lockForWrite();
	bool tryLockForWrite();
	bool tryLockForWrite(int timeout);

	void unlock();
	void unlockForRead();
	void unlockForWrite();

private:
	Q_DISABLE_COPY(WinReadWriteLock)
	SRWLOCK m_lock;
};




class WinReadLocker
{
public:
	inline WinReadLocker(WinReadWriteLock *readWriteLock);

	inline ~WinReadLocker()
	{
		unlock();
	}

	inline void unlock()
	{
		if (q_val) {
			if ((q_val & quintptr(1u)) == quintptr(1u)) {
				q_val &= ~quintptr(1u);
				readWriteLock()->unlock();
			}
		}
	}

	inline void relock()
	{
		if (q_val) {
			if ((q_val & quintptr(1u)) == quintptr(0u)) {
				readWriteLock()->lockForRead();
				q_val |= quintptr(1u);
			}
		}
	}

	inline WinReadWriteLock *readWriteLock() const
	{
		return reinterpret_cast<WinReadWriteLock *>(q_val & ~quintptr(1u));
	}

private:
	Q_DISABLE_COPY(WinReadLocker)
	quintptr q_val;
};

inline WinReadLocker::WinReadLocker(WinReadWriteLock *areadWriteLock)
	: q_val(reinterpret_cast<quintptr>(areadWriteLock))
{
	Q_ASSERT_X((q_val & quintptr(1u)) == quintptr(0),
		"WinReadLocker", "WinReadWriteLock pointer is misaligned");
	relock();
}

class WinWriteLocker
{
public:
	inline WinWriteLocker(WinReadWriteLock *readWriteLock);

	inline ~WinWriteLocker()
	{
		unlock();
	}

	inline void unlock()
	{
		if (q_val) {
			if ((q_val & quintptr(1u)) == quintptr(1u)) {
				q_val &= ~quintptr(1u);
				readWriteLock()->unlock();
			}
		}
	}

	inline void relock()
	{
		if (q_val) {
			if ((q_val & quintptr(1u)) == quintptr(0u)) {
				readWriteLock()->lockForWrite();
				q_val |= quintptr(1u);
			}
		}
	}

	inline WinReadWriteLock *readWriteLock() const
	{
		return reinterpret_cast<WinReadWriteLock *>(q_val & ~quintptr(1u));
	}


private:
	Q_DISABLE_COPY(WinWriteLocker)
	quintptr q_val;
};

inline WinWriteLocker::WinWriteLocker(WinReadWriteLock *areadWriteLock)
	: q_val(reinterpret_cast<quintptr>(areadWriteLock))
{
	Q_ASSERT_X((q_val & quintptr(1u)) == quintptr(0),
		"WinWriteLocker", "WinReadWriteLock pointer is misaligned");
	relock();
}