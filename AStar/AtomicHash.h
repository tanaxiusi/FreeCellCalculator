#pragma once

#include <QtCore/qchar.h>
#include <QtCore/qiterator.h>
#include <QtCore/qlist.h>
#include <QtCore/qrefcount.h>
#include <QtCore/qhashfunctions.h>

#ifdef Q_COMPILER_INITIALIZER_LISTS
#include <initializer_list>
#endif
#include "WinReadWriteLock.h"

#if defined(Q_CC_MSVC)
#pragma warning( push )
#pragma warning( disable : 4311 ) // disable pointer truncation warning
#pragma warning( disable : 4127 ) // conditional expression is constant
#endif

struct AtomicHashData
{
    struct Node {
        Node *next;
        uint h;
    };

    Node *fakeNext;
    Node **buckets;
	WinReadWriteLock * bucketsLock;
	WinReadWriteLock * globalLock;
    QtPrivate::RefCount ref;
    QAtomicInt size;
    int nodeSize;
    short userNumBits;
    short numBits;
    int numBuckets;
    uint seed;
    uint sharable : 1;
    uint strictAlignment : 1;
    uint reserved : 30;

    void *allocateNode(int nodeAlign);
    void freeNode(void *node);
    AtomicHashData *detach_helper(void (*node_duplicate)(Node *, void *), void (*node_delete)(Node *),
                             int nodeSize, int nodeAlign);
    bool willGrow();
	void grow();
    void hasShrunk();
	void shrunk();
    void rehash(int hint);
    void free_helper(void (*node_delete)(Node *));
    Node *firstNode();
#ifdef QT_QHASH_DEBUG
    void dump();
    void checkSanity();
#endif
    static Node *nextNode(Node *node);
    static Node *previousNode(Node *node);

    static const AtomicHashData shared_null;
};

inline bool AtomicHashData::willGrow()
{
    if (size >= numBuckets) {
        rehash(numBits + 1);
        return true;
    } else {
        return false;
    }
}

inline void AtomicHashData::grow()
{
	rehash(numBits + 1);
}

inline void AtomicHashData::hasShrunk()
{
    if (size <= (numBuckets >> 3) && numBits > userNumBits) {
        QT_TRY {
            rehash(qMax(int(numBits) - 2, int(userNumBits)));
        } QT_CATCH(const std::bad_alloc &) {
            // ignore bad allocs - shrinking shouldn't throw. rehash is exception safe.
        }
    }
}

inline void AtomicHashData::shrunk()
{
	QT_TRY{
		rehash(qMax(int(numBits) - 2, int(userNumBits)));
	} QT_CATCH(const std::bad_alloc &) {
		// ignore bad allocs - shrinking shouldn't throw. rehash is exception safe.
	}
}

inline AtomicHashData::Node *AtomicHashData::firstNode()
{
    Node *e = reinterpret_cast<Node *>(this);
    Node **bucket = buckets;
    int n = numBuckets;
    while (n--) {
        if (*bucket != e)
            return *bucket;
        ++bucket;
    }
    return e;
}

struct AtomicHashDummyValue
{
};

inline bool operator==(const AtomicHashDummyValue & /* v1 */, const AtomicHashDummyValue & /* v2 */)
{
    return true;
}

Q_DECLARE_TYPEINFO(AtomicHashDummyValue, Q_MOVABLE_TYPE | Q_DUMMY_TYPE);

template <class Key, class T>
struct AtomicHashNode
{
	AtomicHashNode *next;
    const uint h;
    const Key key;
    T value;

    inline AtomicHashNode(const Key &key0, const T &value0, uint hash, AtomicHashNode *n)
        : next(n), h(hash), key(key0), value(value0) {}
    inline bool same_key(uint h0, const Key &key0) const { return h0 == h && key0 == key; }

private:
    Q_DISABLE_COPY(AtomicHashNode)
};

// Specialize for AtomicHashDummyValue in order to save some memory
template <class Key>
struct AtomicHashNode<Key, AtomicHashDummyValue>
{
    union {
		AtomicHashNode *next;
		AtomicHashDummyValue value;
    };
    const uint h;
    const Key key;

    inline AtomicHashNode(const Key &key0, const AtomicHashDummyValue &, uint hash, AtomicHashNode *n)
        : next(n), h(hash), key(key0) {}
    inline bool same_key(uint h0, const Key &key0) const { return h0 == h && key0 == key; }

private:
    Q_DISABLE_COPY(AtomicHashNode)
};

template <class Key, class T>
class AtomicHash
{
    typedef AtomicHashNode<Key, T> Node;

    union {
		AtomicHashData *d;
		AtomicHashNode<Key, T> *e;
    };

    static inline Node *concrete(AtomicHashData::Node *node) {
        return reinterpret_cast<Node *>(node);
    }

    static inline int alignOfNode() { return qMax<int>(sizeof(void*), Q_ALIGNOF(Node)); }

public:
    inline AtomicHash() Q_DECL_NOTHROW : d(const_cast<AtomicHashData *>(&AtomicHashData::shared_null)) { }
#ifdef Q_COMPILER_INITIALIZER_LISTS
    inline AtomicHash(std::initializer_list<std::pair<Key,T> > list)
        : d(const_cast<AtomicHashData *>(&AtomicHashData::shared_null))
    {
        reserve(int(list.size()));
        for (typename std::initializer_list<std::pair<Key,T> >::const_iterator it = list.begin(); it != list.end(); ++it)
            insert(it->first, it->second);
    }
#endif
	AtomicHash(const AtomicHash &other) : d(other.d) { d->ref.ref(); if (!d->sharable) detach(); }
    ~AtomicHash() { if (!d->ref.deref()) freeData(d); }

	AtomicHash &operator=(const AtomicHash &other);
#ifdef Q_COMPILER_RVALUE_REFS
	AtomicHash(AtomicHash &&other) Q_DECL_NOTHROW : d(other.d) { other.d = const_cast<AtomicHashData *>(&AtomicHashData::shared_null); }
	AtomicHash &operator=(AtomicHash &&other) Q_DECL_NOTHROW
	{
		AtomicHash moved(std::move(other)); swap(moved); return *this;
}
#endif
    void swap(AtomicHash &other) Q_DECL_NOTHROW { qSwap(d, other.d); }

    bool operator==(const AtomicHash &other) const;
    bool operator!=(const AtomicHash &other) const { return !(*this == other); }

    inline int size() const { return d->size; }

    inline bool isEmpty() const { return d->size == 0; }

    inline int capacity() const { return d->numBuckets; }
    void reserve(int size);
    inline void squeeze() { reserve(1); }

    inline void detach() { if (d->ref.isShared()) detach_helper(); }
    inline bool isDetached() const { return !d->ref.isShared(); }
#if !defined(QT_NO_UNSHARABLE_CONTAINERS)
    inline void setSharable(bool sharable) { if (!sharable) detach(); if (d != &AtomicHashData::shared_null) d->sharable = sharable; }
#endif
    bool isSharedWith(const AtomicHash &other) const { return d == other.d; }

    void clear();

    int remove(const Key &key);
    T take(const Key &key);

    bool contains(const Key &key) const;
    const Key key(const T &value) const;
    const Key key(const T &value, const Key &defaultKey) const;
    const T value(const Key &key) const;
    const T value(const Key &key, const T &defaultValue) const;
    T &operator[](const Key &key);
    const T operator[](const Key &key) const;

    QList<Key> uniqueKeys() const;
    QList<Key> keys() const;
    QList<Key> keys(const T &value) const;
    QList<T> values() const;
    QList<T> values(const Key &key) const;
    int count(const Key &key) const;

    class const_iterator;

    class iterator
    {
        friend class const_iterator;
        friend class AtomicHash<Key, T>;
		AtomicHashData::Node *i;

    public:
        typedef std::bidirectional_iterator_tag iterator_category;
        typedef qptrdiff difference_type;
        typedef T value_type;
        typedef T *pointer;
        typedef T &reference;

        inline iterator() : i(Q_NULLPTR) { }
        explicit inline iterator(void *node) : i(reinterpret_cast<AtomicHashData::Node *>(node)) { }

        inline const Key &key() const { return concrete(i)->key; }
        inline T &value() const { return concrete(i)->value; }
        inline T &operator*() const { return concrete(i)->value; }
        inline T *operator->() const { return &concrete(i)->value; }
        inline bool operator==(const iterator &o) const { return i == o.i; }
        inline bool operator!=(const iterator &o) const { return i != o.i; }

        inline iterator &operator++() {
            i = AtomicHashData::nextNode(i);
            return *this;
        }
        inline iterator operator++(int) {
            iterator r = *this;
            i = AtomicHashData::nextNode(i);
            return r;
        }
        inline iterator &operator--() {
            i = AtomicHashData::previousNode(i);
            return *this;
        }
        inline iterator operator--(int) {
            iterator r = *this;
            i = AtomicHashData::previousNode(i);
            return r;
        }
        inline iterator operator+(int j) const
        { iterator r = *this; if (j > 0) while (j--) ++r; else while (j++) --r; return r; }
        inline iterator operator-(int j) const { return operator+(-j); }
        inline iterator &operator+=(int j) { return *this = *this + j; }
        inline iterator &operator-=(int j) { return *this = *this - j; }

#ifndef QT_STRICT_ITERATORS
    public:
        inline bool operator==(const const_iterator &o) const
            { return i == o.i; }
        inline bool operator!=(const const_iterator &o) const
            { return i != o.i; }
#endif
    };
    friend class iterator;

    class const_iterator
    {
        friend class iterator;
        friend class QSet<Key>;
		AtomicHashData::Node *i;

    public:
        typedef std::bidirectional_iterator_tag iterator_category;
        typedef qptrdiff difference_type;
        typedef T value_type;
        typedef const T *pointer;
        typedef const T &reference;

        inline const_iterator() : i(Q_NULLPTR) { }
        explicit inline const_iterator(void *node)
            : i(reinterpret_cast<AtomicHashData::Node *>(node)) { }
#ifdef QT_STRICT_ITERATORS
        explicit inline const_iterator(const iterator &o)
#else
        inline const_iterator(const iterator &o)
#endif
        { i = o.i; }

        inline const Key &key() const { return concrete(i)->key; }
        inline const T &value() const { return concrete(i)->value; }
        inline const T &operator*() const { return concrete(i)->value; }
        inline const T *operator->() const { return &concrete(i)->value; }
        inline bool operator==(const const_iterator &o) const { return i == o.i; }
        inline bool operator!=(const const_iterator &o) const { return i != o.i; }

        inline const_iterator &operator++() {
            i = AtomicHashData::nextNode(i);
            return *this;
        }
        inline const_iterator operator++(int) {
            const_iterator r = *this;
            i = AtomicHashData::nextNode(i);
            return r;
        }
        inline const_iterator &operator--() {
            i = AtomicHashData::previousNode(i);
            return *this;
        }
        inline const_iterator operator--(int) {
            const_iterator r = *this;
            i = AtomicHashData::previousNode(i);
            return r;
        }
        inline const_iterator operator+(int j) const
        { const_iterator r = *this; if (j > 0) while (j--) ++r; else while (j++) --r; return r; }
        inline const_iterator operator-(int j) const { return operator+(-j); }
        inline const_iterator &operator+=(int j) { return *this = *this + j; }
        inline const_iterator &operator-=(int j) { return *this = *this - j; }

        // ### Qt 5: not sure this is necessary anymore
#ifdef QT_STRICT_ITERATORS
    private:
        inline bool operator==(const iterator &o) const { return operator==(const_iterator(o)); }
        inline bool operator!=(const iterator &o) const { return operator!=(const_iterator(o)); }
#endif
    };
    friend class const_iterator;

    class key_iterator
    {
        const_iterator i;

    public:
        typedef typename const_iterator::iterator_category iterator_category;
        typedef typename const_iterator::difference_type difference_type;
        typedef Key value_type;
        typedef const Key *pointer;
        typedef const Key &reference;

        explicit key_iterator(const_iterator o) : i(o) { }

        const Key &operator*() const { return i.key(); }
        const Key *operator->() const { return &i.key(); }
        bool operator==(key_iterator o) const { return i == o.i; }
        bool operator!=(key_iterator o) const { return i != o.i; }

        inline key_iterator &operator++() { ++i; return *this; }
        inline key_iterator operator++(int) { return key_iterator(i++);}
        inline key_iterator &operator--() { --i; return *this; }
        inline key_iterator operator--(int) { return key_iterator(i--); }
        const_iterator base() const { return i; }
    };

    // STL style
    inline iterator begin() { detach(); return iterator(d->firstNode()); }
    inline const_iterator begin() const { return const_iterator(d->firstNode()); }
    inline const_iterator cbegin() const { return const_iterator(d->firstNode()); }
    inline const_iterator constBegin() const { return const_iterator(d->firstNode()); }
    inline iterator end() { detach(); return iterator(e); }
    inline const_iterator end() const { return const_iterator(e); }
    inline const_iterator cend() const { return const_iterator(e); }
    inline const_iterator constEnd() const { return const_iterator(e); }
    inline key_iterator keyBegin() const { return key_iterator(begin()); }
    inline key_iterator keyEnd() const { return key_iterator(end()); }

    iterator erase(iterator it);

    // more Qt
    typedef iterator Iterator;
    typedef const_iterator ConstIterator;
    inline int count() const { return d->size; }
    iterator find(const Key &key);
    const_iterator find(const Key &key) const;
    const_iterator constFind(const Key &key) const;
    iterator insert(const Key &key, const T &value);
    iterator insertMulti(const Key &key, const T &value);
	AtomicHash &unite(const AtomicHash &other);

    // STL compatibility
    typedef T mapped_type;
    typedef Key key_type;
    typedef qptrdiff difference_type;
    typedef int size_type;

    inline bool empty() const { return isEmpty(); }

#ifdef QT_QHASH_DEBUG
    inline void dump() const { d->dump(); }
    inline void checkSanity() const { d->checkSanity(); }
#endif

	iterator atomicFind(const Key & key)
	{
		return iterator(*atomicFindNode(key, qHash(key, d->seed)));
	}
	iterator atomicCreate(const Key & key, const T & value = T(), bool replace = false, bool * created = 0)
	{
		detach();
		return iterator(atomicCreateNode(key, qHash(key, d->seed), value, replace, created));
	}
	int atomicRemove(const Key & key)
	{
		if (isEmpty()) // prevents detaching shared null
			return 0;
		detach();
		return atomicDeleteNode(key, qHash(key, d->seed));
	}

private:
    void detach_helper();
    void freeData(AtomicHashData *d);
    Node **findNode(const Key &key, uint *hp = Q_NULLPTR) const;
    Node **findNode(const Key &key, uint h) const;
    Node *createNode(uint h, const Key &key, const T &value, Node **nextNode);
    void deleteNode(Node *node);
    static void deleteNode2(AtomicHashData::Node *node);

    static void duplicateNode(AtomicHashData::Node *originalNode, void *newNode);

	Node ** atomicFindNode(const Key &key, uint h) const;
	Node * atomicCreateNode(const Key & key, uint h, const T & value, bool replace, bool * created);
	int atomicDeleteNode(const Key & key, uint h);

	Node * createNodeNotResize(uint ah, const Key &akey, const T &avalue, Node **anextNode);

    bool isValidIterator(const iterator &it) const
    {
#if defined(QT_DEBUG) && !defined(Q_HASH_NO_ITERATOR_DEBUG)
        AtomicHashData::Node *node = it.i;
        while (node->next)
            node = node->next;
        return (static_cast<void *>(node) == d);
#else
        Q_UNUSED(it);
        return true;
#endif
    }
    friend class QSet<Key>;
};


template <class Key, class T>
Q_INLINE_TEMPLATE void AtomicHash<Key, T>::deleteNode(Node *node)
{
    deleteNode2(reinterpret_cast<AtomicHashData::Node*>(node));
    d->freeNode(node);
}

template <class Key, class T>
Q_INLINE_TEMPLATE void AtomicHash<Key, T>::deleteNode2(AtomicHashData::Node *node)
{
#ifdef Q_CC_BOR
    concrete(node)->~AtomicHashNode<Key, T>();
#else
    concrete(node)->~Node();
#endif
}

template <class Key, class T>
Q_INLINE_TEMPLATE void AtomicHash<Key, T>::duplicateNode(AtomicHashData::Node *node, void *newNode)
{
    Node *concreteNode = concrete(node);
    new (newNode) Node(concreteNode->key, concreteNode->value, concreteNode->h, Q_NULLPTR);
}


template <class Key, class T>
Q_INLINE_TEMPLATE typename AtomicHash<Key, T>::Node *
AtomicHash<Key, T>::createNode(uint ah, const Key &akey, const T &avalue, Node **anextNode)
{
    Node *node = new (d->allocateNode(alignOfNode())) Node(akey, avalue, ah, *anextNode);
    *anextNode = node;
    ++d->size;
    return node;
}

template <class Key, class T>
Q_INLINE_TEMPLATE AtomicHash<Key, T> &AtomicHash<Key, T>::unite(const AtomicHash &other)
{
    AtomicHash copy(other);
    const_iterator it = copy.constEnd();
    while (it != copy.constBegin()) {
        --it;
        insertMulti(it.key(), it.value());
    }
    return *this;
}

template <class Key, class T>
Q_OUTOFLINE_TEMPLATE void AtomicHash<Key, T>::freeData(AtomicHashData *x)
{
    x->free_helper(deleteNode2);
}

template <class Key, class T>
Q_INLINE_TEMPLATE void AtomicHash<Key, T>::clear()
{
    *this = AtomicHash();
}

template <class Key, class T>
Q_OUTOFLINE_TEMPLATE void AtomicHash<Key, T>::detach_helper()
{
    AtomicHashData *x = d->detach_helper(duplicateNode, deleteNode2, sizeof(Node), alignOfNode());
    if (!d->ref.deref())
        freeData(d);
    d = x;
}

template <class Key, class T>
Q_INLINE_TEMPLATE AtomicHash<Key, T> &AtomicHash<Key, T>::operator=(const AtomicHash &other)
{
    if (d != other.d) {
        AtomicHashData *o = other.d;
        o->ref.ref();
        if (!d->ref.deref())
            freeData(d);
        d = o;
        if (!d->sharable)
            detach_helper();
    }
    return *this;
}

template <class Key, class T>
Q_INLINE_TEMPLATE const T AtomicHash<Key, T>::value(const Key &akey) const
{
    Node *node;
    if (d->size == 0 || (node = *findNode(akey)) == e) {
        return T();
    } else {
        return node->value;
    }
}

template <class Key, class T>
Q_INLINE_TEMPLATE const T AtomicHash<Key, T>::value(const Key &akey, const T &adefaultValue) const
{
    Node *node;
    if (d->size == 0 || (node = *findNode(akey)) == e) {
        return adefaultValue;
    } else {
        return node->value;
    }
}

template <class Key, class T>
Q_OUTOFLINE_TEMPLATE QList<Key> AtomicHash<Key, T>::uniqueKeys() const
{
    QList<Key> res;
    res.reserve(size()); // May be too much, but assume short lifetime
    const_iterator i = begin();
    if (i != end()) {
        for (;;) {
            const Key &aKey = i.key();
            res.append(aKey);
            do {
                if (++i == end())
                    goto break_out_of_outer_loop;
            } while (aKey == i.key());
        }
    }
break_out_of_outer_loop:
    return res;
}

template <class Key, class T>
Q_OUTOFLINE_TEMPLATE QList<Key> AtomicHash<Key, T>::keys() const
{
    QList<Key> res;
    res.reserve(size());
    const_iterator i = begin();
    while (i != end()) {
        res.append(i.key());
        ++i;
    }
    return res;
}

template <class Key, class T>
Q_OUTOFLINE_TEMPLATE QList<Key> AtomicHash<Key, T>::keys(const T &avalue) const
{
    QList<Key> res;
    const_iterator i = begin();
    while (i != end()) {
        if (i.value() == avalue)
            res.append(i.key());
        ++i;
    }
    return res;
}

template <class Key, class T>
Q_OUTOFLINE_TEMPLATE const Key AtomicHash<Key, T>::key(const T &avalue) const
{
    return key(avalue, Key());
}

template <class Key, class T>
Q_OUTOFLINE_TEMPLATE const Key AtomicHash<Key, T>::key(const T &avalue, const Key &defaultValue) const
{
    const_iterator i = begin();
    while (i != end()) {
        if (i.value() == avalue)
            return i.key();
        ++i;
    }

    return defaultValue;
}

template <class Key, class T>
Q_OUTOFLINE_TEMPLATE QList<T> AtomicHash<Key, T>::values() const
{
    QList<T> res;
    res.reserve(size());
    const_iterator i = begin();
    while (i != end()) {
        res.append(i.value());
        ++i;
    }
    return res;
}

template <class Key, class T>
Q_OUTOFLINE_TEMPLATE QList<T> AtomicHash<Key, T>::values(const Key &akey) const
{
    QList<T> res;
    Node *node = *findNode(akey);
    if (node != e) {
        do {
            res.append(node->value);
        } while ((node = node->next) != e && node->key == akey);
    }
    return res;
}

template <class Key, class T>
Q_OUTOFLINE_TEMPLATE int AtomicHash<Key, T>::count(const Key &akey) const
{
    int cnt = 0;
    Node *node = *findNode(akey);
    if (node != e) {
        do {
            ++cnt;
        } while ((node = node->next) != e && node->key == akey);
    }
    return cnt;
}

template <class Key, class T>
Q_INLINE_TEMPLATE const T AtomicHash<Key, T>::operator[](const Key &akey) const
{
    return value(akey);
}

template <class Key, class T>
Q_INLINE_TEMPLATE T &AtomicHash<Key, T>::operator[](const Key &akey)
{
    detach();

    uint h;
    Node **node = findNode(akey, &h);
    if (*node == e) {
        if (d->willGrow())
            node = findNode(akey, &h);
        return createNode(h, akey, T(), node)->value;
    }
    return (*node)->value;
}

template <class Key, class T>
Q_INLINE_TEMPLATE typename AtomicHash<Key, T>::iterator AtomicHash<Key, T>::insert(const Key &akey,
                                                                         const T &avalue)
{
    detach();

    uint h;
    Node **node = findNode(akey, &h);
    if (*node == e) {
        if (d->willGrow())
            node = findNode(akey, &h);
        return iterator(createNode(h, akey, avalue, node));
    }

    if (!QtPrivate::is_same<T, AtomicHashDummyValue>::value)
        (*node)->value = avalue;
    return iterator(*node);
}

template <class Key, class T>
Q_INLINE_TEMPLATE typename AtomicHash<Key, T>::iterator AtomicHash<Key, T>::insertMulti(const Key &akey,
                                                                              const T &avalue)
{
    detach();
    d->willGrow();

    uint h;
    Node **nextNode = findNode(akey, &h);
    return iterator(createNode(h, akey, avalue, nextNode));
}

template <class Key, class T>
Q_OUTOFLINE_TEMPLATE int AtomicHash<Key, T>::remove(const Key &akey)
{
    if (isEmpty()) // prevents detaching shared null
        return 0;
    detach();

    int oldSize = d->size;
    Node **node = findNode(akey);
    if (*node != e) {
        bool deleteNext = true;
        do {
            Node *next = (*node)->next;
            deleteNext = (next != e && next->key == (*node)->key);
            deleteNode(*node);
            *node = next;
            --d->size;
        } while (deleteNext);
        d->hasShrunk();
    }
    return oldSize - d->size;
}

template <class Key, class T>
Q_OUTOFLINE_TEMPLATE T AtomicHash<Key, T>::take(const Key &akey)
{
    if (isEmpty()) // prevents detaching shared null
        return T();
    detach();

    Node **node = findNode(akey);
    if (*node != e) {
        T t = (*node)->value;
        Node *next = (*node)->next;
        deleteNode(*node);
        *node = next;
        --d->size;
        d->hasShrunk();
        return t;
    }
    return T();
}

template <class Key, class T>
Q_OUTOFLINE_TEMPLATE typename AtomicHash<Key, T>::iterator AtomicHash<Key, T>::erase(iterator it)
{
    Q_ASSERT_X(isValidIterator(it), "AtomicHash::erase", "The specified iterator argument 'it' is invalid");

    if (it == iterator(e))
        return it;

    if (d->ref.isShared()) {
        int bucketNum = (it.i->h % d->numBuckets);
        iterator bucketIterator(*(d->buckets + bucketNum));
        int stepsFromBucketStartToIte = 0;
        while (bucketIterator != it) {
            ++stepsFromBucketStartToIte;
            ++bucketIterator;
        }
        detach();
        it = iterator(*(d->buckets + bucketNum));
        while (stepsFromBucketStartToIte > 0) {
            --stepsFromBucketStartToIte;
            ++it;
        }
    }

    iterator ret = it;
    ++ret;

    Node *node = concrete(it.i);
    Node **node_ptr = reinterpret_cast<Node **>(&d->buckets[node->h % d->numBuckets]);
    while (*node_ptr != node)
        node_ptr = &(*node_ptr)->next;
    *node_ptr = node->next;
    deleteNode(node);
    --d->size;
    return ret;
}

template <class Key, class T>
Q_INLINE_TEMPLATE void AtomicHash<Key, T>::reserve(int asize)
{
    detach();
    d->rehash(-qMax(asize, 1));
}

template <class Key, class T>
Q_INLINE_TEMPLATE typename AtomicHash<Key, T>::const_iterator AtomicHash<Key, T>::find(const Key &akey) const
{
    return const_iterator(*findNode(akey));
}

template <class Key, class T>
Q_INLINE_TEMPLATE typename AtomicHash<Key, T>::const_iterator AtomicHash<Key, T>::constFind(const Key &akey) const
{
    return const_iterator(*findNode(akey));
}

template <class Key, class T>
Q_INLINE_TEMPLATE typename AtomicHash<Key, T>::iterator AtomicHash<Key, T>::find(const Key &akey)
{
    detach();
    return iterator(*findNode(akey));
}

template <class Key, class T>
Q_INLINE_TEMPLATE bool AtomicHash<Key, T>::contains(const Key &akey) const
{
    return *findNode(akey) != e;
}

template <class Key, class T>
Q_OUTOFLINE_TEMPLATE typename AtomicHash<Key, T>::Node **AtomicHash<Key, T>::findNode(const Key &akey, uint h) const
{
    Node **node;

    if (d->numBuckets) {
        node = reinterpret_cast<Node **>(&d->buckets[h % d->numBuckets]);
        Q_ASSERT(*node == e || (*node)->next);
        while (*node != e && !(*node)->same_key(h, akey))
            node = &(*node)->next;
    } else {
        node = const_cast<Node **>(reinterpret_cast<const Node * const *>(&e));
    }
    return node;
}

template <class Key, class T>
Q_OUTOFLINE_TEMPLATE typename AtomicHash<Key, T>::Node **AtomicHash<Key, T>::findNode(const Key &akey,
                                                                            uint *ahp) const
{
    uint h = 0;

    if (d->numBuckets || ahp) {
        h = qHash(akey, d->seed);
        if (ahp)
            *ahp = h;
    }
    return findNode(akey, h);
}



template <class Key, class T>
typename AtomicHash<Key, T>::Node ** AtomicHash<Key, T>::atomicFindNode(const Key &key, uint h) const
{
	Node ** node;
	d->globalLock->lockForRead();
	if (d->numBuckets != 0)
	{
		d->bucketsLock[h % d->numBuckets].lockForRead();
		node = findNode(key, h);
		d->bucketsLock[h % d->numBuckets].unlock();
	}
	else
	{
		node = const_cast<Node **>(reinterpret_cast<const Node * const *>(&e));
	}
	d->globalLock->unlock();
	return node;
}
template <class Key, class T>
typename AtomicHash<Key, T>::Node * AtomicHash<Key, T>::atomicCreateNode(const Key & key, uint h, const T & value, bool replace, bool * created)
{
	bool dummy;
	if (!created)
		created = &dummy;

	d->globalLock->lockForRead();

	if (d->numBuckets == 0)
	{
		d->globalLock->unlock();

		d->globalLock->lockForWrite();
		if (d->numBuckets == 0)
			d->grow();
	}

	d->bucketsLock[h % d->numBuckets].lockForWrite();
	Node **nodePtr = findNode(key, h);
	if (*nodePtr == e)
	{
		Node * node = createNodeNotResize(h, key, value, nodePtr);
		d->bucketsLock[h % d->numBuckets].unlock();
		d->globalLock->unlock();

		if (d->size.fetchAndAddRelaxed(1) + 1 >= d->numBuckets)
		{
			d->globalLock->lockForWrite();
			d->grow();
			d->globalLock->unlock();
		}

		*created = true;
		return node;
	}else
	{
		Node * node = *nodePtr;
		if (replace)
			node->value = value;
		d->bucketsLock[h % d->numBuckets].unlock();
		d->globalLock->unlock();

		*created = false;
		return node;
	}
}


template <class Key, class T>
int AtomicHash<Key, T>::atomicDeleteNode(const Key & key, uint h)
{
	int removed = 0;

	d->globalLock->lockForRead();
	d->bucketsLock[h % d->numBuckets].lockForWrite();

	Node **node = findNode(key, h);
	if (*node != e) {
		bool deleteNext = true;
		do {
			Node *next = (*node)->next;
			deleteNext = (next != e && next->key == (*node)->key);
			deleteNode(*node);
			*node = next;
			removed++;
		} while (deleteNext);

		d->bucketsLock[h % d->numBuckets].unlock();
		d->globalLock->unlock();

		const int afterSize = d->size.fetchAndAddRelaxed(-removed) - removed;

		if (afterSize <= (d->numBuckets >> 3) && d->numBits > d->userNumBits)
		{
			d->globalLock->lockForWrite();
			d->shrunk();
			d->globalLock->unlock();
		}
	}
	else
	{
		d->bucketsLock[h % d->numBuckets].unlock();
		d->globalLock->unlock();
	}

	return removed;
}


template <class Key, class T>
typename AtomicHash<Key, T>::Node *
AtomicHash<Key, T>::createNodeNotResize(uint ah, const Key &akey, const T &avalue, Node **anextNode)
{
	Node *node = new (d->allocateNode(alignOfNode())) Node(akey, avalue, ah, *anextNode);
	*anextNode = node;
	return node;
}

template <class Key, class T>
Q_OUTOFLINE_TEMPLATE bool AtomicHash<Key, T>::operator==(const AtomicHash &other) const
{
    if (size() != other.size())
        return false;
    if (d == other.d)
        return true;

    const_iterator it = begin();

    while (it != end()) {
        const Key &akey = it.key();

        const_iterator it2 = other.find(akey);
        do {
            if (it2 == other.end() || !(it2.key() == akey))
                return false;
            if (!(it.value() == it2.value()))
                return false;
            ++it;
            ++it2;
        } while (it != end() && it.key() == akey);
    }
    return true;
}

//Q_DECLARE_ASSOCIATIVE_ITERATOR(Hash)
//Q_DECLARE_MUTABLE_ASSOCIATIVE_ITERATOR(Hash)

#if defined(Q_CC_MSVC)
#pragma warning( pop )
#endif
