
#include <stdlib.h>

#include "AHash.h"

#ifdef truncate
#undef truncate
#endif

#include <qbitarray.h>
#include <qstring.h>
#include <qglobal.h>
#include <qbytearray.h>
#include <qdatetime.h>
#include <qbasicatomic.h>
#include <qendian.h>

#ifndef QT_BOOTSTRAPPED
#include <qcoreapplication.h>
#endif // QT_BOOTSTRAPPED

#ifdef Q_OS_UNIX
#include <stdio.h>
#include "private/qcore_unix_p.h"
#endif // Q_OS_UNIX

#include <limits.h>


/*!
\internal

Creates the QHash random seed from various sources.
In order of decreasing precedence:
- under Unix, it attemps to read from /dev/urandom;
- under Unix, it attemps to read from /dev/random;
- under Windows, it attempts to use rand_s;
- as a general fallback, the application's PID, a timestamp and the
address of a stack-local variable are used.
*/

static uint qt_create_qhash_seed()
{
	uint seed = 0;

#ifndef QT_BOOTSTRAPPED
	QByteArray envSeed = qgetenv("QT_HASH_SEED");
	if (!envSeed.isNull())
		return envSeed.toUInt();

#ifdef Q_OS_UNIX
	int randomfd = qt_safe_open("/dev/urandom", O_RDONLY);
	if (randomfd == -1)
		randomfd = qt_safe_open("/dev/random", O_RDONLY | O_NONBLOCK);
	if (randomfd != -1) {
		if (qt_safe_read(randomfd, reinterpret_cast<char *>(&seed), sizeof(seed)) == sizeof(seed)) {
			qt_safe_close(randomfd);
			return seed;
		}
		qt_safe_close(randomfd);
	}
#endif // Q_OS_UNIX

#if defined(Q_OS_WIN32) && !defined(Q_CC_GNU)
	errno_t err = 0;
	seed = rand();
	if (err == 0)
		return seed;
#endif // Q_OS_WIN32

	// general fallback: initialize from the current timestamp, pid,
	// and address of a stack-local variable
	quint64 timestamp = QDateTime::currentMSecsSinceEpoch();
	seed ^= timestamp;
	seed ^= (timestamp >> 32);

	quint64 pid = QCoreApplication::applicationPid();
	seed ^= pid;
	seed ^= (pid >> 32);

	quintptr seedPtr = reinterpret_cast<quintptr>(&seed);
	seed ^= seedPtr;
	seed ^= (qulonglong(seedPtr) >> 32); // no-op on 32-bit platforms
#endif // QT_BOOTSTRAPPED

	return seed;
}

/*
The QHash seed itself.
*/
static QBasicAtomicInt qt_qhash_seed = Q_BASIC_ATOMIC_INITIALIZER(-1);

/*!
\internal

Seed == -1 means it that it was not initialized yet.

We let qt_create_qhash_seed return any unsigned integer,
but convert it to signed in order to initialize the seed.

We don't actually care about the fact that different calls to
qt_create_qhash_seed() might return different values,
as long as in the end everyone uses the very same value.
*/
static void qt_initialize_qhash_seed()
{
	if (qt_qhash_seed.load() == -1) {
		int x(qt_create_qhash_seed() & INT_MAX);
		qt_qhash_seed.testAndSetRelaxed(-1, x);
	}
}

/*!
\internal

Private copy of the implementation of the Qt 4 qHash algorithm for strings,
(that is, QChar-based arrays, so all QString-like classes),
to be used wherever the result is somehow stored or reused across multiple
Qt versions. The public qHash implementation can change at any time,
therefore one must not rely on the fact that it will always give the same
results.

/*
The prime_deltas array contains the difference between a power
of two and the next prime number:

prime_deltas[i] = nextprime(2^i) - 2^i

Basically, it's sequence A092131 from OEIS, assuming:
- nextprime(1) = 1
- nextprime(2) = 2
and
- left-extending it for the offset 0 (A092131 starts at i=1)
- stopping the sequence at i = 28 (the table is big enough...)
*/

static const uchar prime_deltas[] = {
	0,  0,  1,  3,  1,  5,  3,  3,  1,  9,  7,  5,  3, 17, 27,  3,
	1, 29,  3, 21,  7, 17, 15,  9, 43, 35, 15,  0,  0,  0,  0,  0
};

/*
The primeForNumBits() function returns the prime associated to a
power of two. For example, primeForNumBits(8) returns 257.
*/

static inline int primeForNumBits(int numBits)
{
	return (1 << numBits) + prime_deltas[numBits];
}

/*
Returns the smallest integer n such that
primeForNumBits(n) >= hint.
*/
static int countBits(int hint)
{
	int numBits = 0;
	int bits = hint;

	while (bits > 1) {
		bits >>= 1;
		numBits++;
	}

	if (numBits >= (int)sizeof(prime_deltas)) {
		numBits = sizeof(prime_deltas) - 1;
	}
	else if (primeForNumBits(numBits) < hint) {
		++numBits;
	}
	return numBits;
}

/*
    A AHash has initially around pow(2, MinNumBits) buckets. For
    example, if MinNumBits is 4, it has 17 buckets.
*/
const int MinNumBits = 4;

const AHashData AHashData::shared_null = {
    0, 0, 0, 0, Q_REFCOUNT_INITIALIZE_STATIC, 0, 0, MinNumBits, 0, 0, 0, true, false, 0
};

void *AHashData::allocateNode(int nodeAlign)
{
    void *ptr = strictAlignment ? qMallocAligned(nodeSize, nodeAlign) : malloc(nodeSize);
    Q_CHECK_PTR(ptr);
    return ptr;
}

void AHashData::freeNode(void *node)
{
    if (strictAlignment)
        qFreeAligned(node);
    else
        free(node);
}

AHashData *AHashData::detach_helper(void (*node_duplicate)(Node *, void *),
                                    void (*node_delete)(Node *),
                                    int nodeSize,
                                    int nodeAlign)
{
    union {
        AHashData *d;
        Node *e;
    };
    if (this == &shared_null)
        qt_initialize_qhash_seed(); // may throw
    d = new AHashData;
    d->fakeNext = 0;
    d->buckets = 0;
	d->bucketsLock = 0;
	d->globalLock = new WinReadWriteLock();
    d->ref.initializeOwned();
    d->size = size;
    d->nodeSize = nodeSize;
    d->userNumBits = userNumBits;
    d->numBits = numBits;
    d->numBuckets = numBuckets;
    d->seed = (this == &shared_null) ? uint(qt_qhash_seed.load()) : seed;
    d->sharable = true;
    d->strictAlignment = nodeAlign > 8;
    d->reserved = 0;

    if (numBuckets) {
        QT_TRY {
            d->buckets = new Node *[numBuckets];
			d->bucketsLock = new WinReadWriteLock[numBuckets];
        } QT_CATCH(...) {
            // restore a consistent state for d
            d->numBuckets = 0;
            // roll back
            d->free_helper(node_delete);
            QT_RETHROW;
        }

        Node *this_e = reinterpret_cast<Node *>(this);
        for (int i = 0; i < numBuckets; ++i) {
            Node **nextNode = &d->buckets[i];
            Node *oldNode = buckets[i];
            while (oldNode != this_e) {
                QT_TRY {
                    Node *dup = static_cast<Node *>(allocateNode(nodeAlign));

                    QT_TRY {
                        node_duplicate(oldNode, dup);
                    } QT_CATCH(...) {
                        freeNode( dup );
                        QT_RETHROW;
                    }

                    *nextNode = dup;
                    nextNode = &dup->next;
                    oldNode = oldNode->next;
                } QT_CATCH(...) {
                    // restore a consistent state for d
                    *nextNode = e;
                    d->numBuckets = i+1;
                    // roll back
                    d->free_helper(node_delete);
                    QT_RETHROW;
                }
            }
            *nextNode = e;
        }
    }
    return d;
}

void AHashData::free_helper(void (*node_delete)(Node *))
{
    if (node_delete) {
        Node *this_e = reinterpret_cast<Node *>(this);
        Node **bucket = reinterpret_cast<Node **>(this->buckets);

        int n = numBuckets;
        while (n--) {
            Node *cur = *bucket++;
            while (cur != this_e) {
                Node *next = cur->next;
                node_delete(cur);
                freeNode(cur);
                cur = next;
            }
        }
    }
    delete [] buckets;
	delete [] bucketsLock;
	delete globalLock;
    delete this;
}

AHashData::Node *AHashData::nextNode(Node *node)
{
    union {
        Node *next;
        Node *e;
        AHashData *d;
    };
    next = node->next;
    Q_ASSERT_X(next, "AHash", "Iterating beyond end()");
    if (next->next)
        return next;

    int start = (node->h % d->numBuckets) + 1;
    Node **bucket = d->buckets + start;
    int n = d->numBuckets - start;
    while (n--) {
        if (*bucket != e)
            return *bucket;
        ++bucket;
    }
    return e;
}

AHashData::Node *AHashData::previousNode(Node *node)
{
    union {
        Node *e;
        AHashData *d;
    };

    e = node;
    while (e->next)
        e = e->next;

    int start;
    if (node == e)
        start = d->numBuckets - 1;
    else
        start = node->h % d->numBuckets;

    Node *sentinel = node;
    Node **bucket = d->buckets + start;
    while (start >= 0) {
        if (*bucket != sentinel) {
            Node *prev = *bucket;
            while (prev->next != sentinel)
                prev = prev->next;
            return prev;
        }

        sentinel = e;
        --bucket;
        --start;
    }
    Q_ASSERT_X(start >= 0, "AHash", "Iterating backward beyond begin()");
    return e;
}

/*
    If hint is negative, -hint gives the approximate number of
    buckets that should be used for the hash table. If hint is
    nonnegative, (1 << hint) gives the approximate number
    of buckets that should be used.
*/
void AHashData::rehash(int hint)
{
    if (hint < 0) {
        hint = countBits(-hint);
        if (hint < MinNumBits)
            hint = MinNumBits;
        userNumBits = hint;
        while (primeForNumBits(hint) < (size >> 1))
            ++hint;
    } else if (hint < MinNumBits) {
        hint = MinNumBits;
    }

    if (numBits != hint) {
        Node *e = reinterpret_cast<Node *>(this);
        Node **oldBuckets = buckets;
		WinReadWriteLock * oldBucketsLock = bucketsLock;
        int oldNumBuckets = numBuckets;

        int nb = primeForNumBits(hint);
        buckets = new Node *[nb];
		bucketsLock = new WinReadWriteLock[nb];
        numBits = hint;
        numBuckets = nb;
        for (int i = 0; i < numBuckets; ++i)
            buckets[i] = e;

        for (int i = 0; i < oldNumBuckets; ++i) {
            Node *firstNode = oldBuckets[i];
            while (firstNode != e) {
                uint h = firstNode->h;
                Node *lastNode = firstNode;
                while (lastNode->next != e && lastNode->next->h == h)
                    lastNode = lastNode->next;

                Node *afterLastNode = lastNode->next;
                Node **beforeFirstNode = &buckets[h % numBuckets];
                while (*beforeFirstNode != e)
                    beforeFirstNode = &(*beforeFirstNode)->next;
                lastNode->next = *beforeFirstNode;
                *beforeFirstNode = firstNode;
                firstNode = afterLastNode;
            }
        }
        delete [] oldBuckets;
		delete [] oldBucketsLock;
    }
}

