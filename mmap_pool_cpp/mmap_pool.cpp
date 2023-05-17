#ifndef MMAP_POOL_HPP
#define MMAP_POOL_HPP

#include <climits>
#include <cstddef>
#include <bits/stdc++.h>

template <typename T, size_t BlockSize = 4096>
class MemoryPool
{
public:
    MemoryPool();
    MemoryPool(const MemoryPool &);
    MemoryPool(MemoryPool &&);
    template <class U>
    MemoryPool(const MemoryPool<U> &);

    ~MemoryPool();

    MemoryPool &operator=(const MemoryPool &) = delete;
    MemoryPool &operator=(MemoryPool &&);

    T *address(T &) const;
    const T *address(const T &) const;

    T *allocate(size_t n = 1, const T *hint = 0);
    void deallocate(T *p, size_t n = 1);

    size_t max_size() const;

    template <class U, class... Args>
    void construct(U *p, Args &&...args);
    template <class U>
    void destroy(U *p);

    template <class... Args>
    T *newElement(Args &&...);
    void deleteElement(T *p);

public:
    template <typename U>
    struct rebind
    {
        typedef MemoryPool<U> other;
    };

private:
    size_t padPointer(char *p, size_t align) const;
    void allocateBlock();

private:
    union Slot_
    {
        T element;
        Slot_ *next;
    };

    Slot_ *currentBlock_;
    Slot_ *currentSlot_;
    Slot_ *lastSlot_;
    Slot_ *freeSlots_;
};

template <typename T, size_t BlockSize>
MemoryPool<T, BlockSize>::MemoryPool()
{
    currentBlock_ = nullptr;
    currentSlot_ = nullptr;
    lastSlot_ = nullptr;
    freeSlots_ = nullptr;
}

template <typename T, size_t BlockSize>
MemoryPool<T, BlockSize>::MemoryPool(const MemoryPool &memoryPool) {}

template <typename T, size_t BlockSize>
MemoryPool<T, BlockSize>::MemoryPool(MemoryPool &&memoryPool)
{
    currentBlock_ = memoryPool.currentBlock_;
    memoryPool.currentBlock_ = nullptr;
    currentSlot_ = memoryPool.currentSlot_;
    lastSlot_ = memoryPool.lastSlot_;
    freeSlots_ = memoryPool.freeSlots_;
}

template <typename T, size_t BlockSize>
template <class U>
MemoryPool<T, BlockSize>::MemoryPool(const MemoryPool<U> &memoryPool) : MemoryPool() {}

template <typename T, size_t BlockSize>
MemoryPool<T, BlockSize> &MemoryPool<T, BlockSize>::operator=(MemoryPool &&memoryPool)
{
    if (this != &memoryPool)
    {
        std::swap(currentBlock_, memoryPool.currentBlock_);
        currentSlot_ = memoryPool.currentSlot_;
        lastSlot_ = memoryPool.lastSlot_;
        freeSlots_ = memoryPool.freeSlots_;
    }
    return *this;
}

template <typename T, size_t BlockSize>
MemoryPool<T, BlockSize>::~MemoryPool()
{
    Slot_ *curr = currentBlock_;
    while (curr != nullptr)
    {
        Slot_ *prev = curr->next;
        operator delete(reinterpret_cast<void *>(curr));
        curr = prev;
    }
}

template <typename T, size_t BlockSize>
T *MemoryPool<T, BlockSize>::address(T &x) const
{
    return &x;
}

template <typename T, size_t BlockSize>
const T *MemoryPool<T, BlockSize>::address(const T &x) const
{
    return &x;
}

template <typename T, size_t BlockSize>
T *MemoryPool<T, BlockSize>::allocate(size_t n, const T *hint)
{
    if (freeSlots_ != nullptr)
    {
        T *result = reinterpret_cast<T *>(freeSlots_);
        freeSlots_ = freeSlots_->next;
        return result;
    }
    else
    {
        if (currentSlot_ >= lastSlot_)
        {
            allocateBlock();
        }
        return reinterpret_cast<T *>(currentSlot_);
    }
}

template <typename T, size_t BlockSize>
void MemoryPool<T, BlockSize>::deallocate(T *p, size_t n)
{
    if (p != nullptr)
    {
        reinterpret_cast<Slot_ *>(p)->next = freeSlots_;
        freeSlots_ = reinterpret_cast<Slot_ *>(p);
    }
}

template <typename T, size_t BlockSize>
size_t MemoryPool<T, BlockSize>::max_size() const
{
    size_t maxBlocks = -1 / BlockSize;
    return (BlockSize - sizeof(T *)) / sizeof(Slot_) * maxBlocks;
}

template <typename T, size_t BlockSize>
template <class U, class... Args>
void MemoryPool<T, BlockSize>::construct(U *p, Args &&...args)
{
    new (p) U(std::forward<Args>(args)...);
}

template <typename T, size_t BlockSize>
template <class U>
void MemoryPool<T, BlockSize>::destroy(U *p)
{
    p->~U();
}

template <typename T, size_t BlockSize>
template <class... Args>
T *MemoryPool<T, BlockSize>::newElement(Args &&...args)
{
    T *result = allocate();
    construct<T>(result, std::forward<Args>(args)...);
    return result;
}
template <typename T, size_t BlockSize>
void MemoryPool<T, BlockSize>::deleteElement(T *p)
{
    if (p != nullptr)
    {
        p->~T();
        deallocate(p);
    }
}

template <typename T, size_t BlockSize>
size_t MemoryPool<T, BlockSize>::padPointer(char *p, size_t align) const
{
    uintptr_t result = reinterpret_cast<uintptr_t>(p);
    return ((align - result) % align);
}

template <typename T, size_t BlockSize>
void MemoryPool<T, BlockSize>::allocateBlock()
{
    char *newBlock = reinterpret_cast<char *>(operator new(BlockSize));
    reinterpret_cast<Slot_ *>(newBlock)->next = currentBlock_;
    currentBlock_ = reinterpret_cast<Slot_ *>(newBlock);
    char *body = newBlock + sizeof(Slot_ *);
    size_t bodyPadding = padPointer(body, alignof(Slot_));
    currentBlock_ = reinterpret_cast<Slot_ *>(body + bodyPadding);
    lastSlot_ = reinterpret_cast<Slot_ *>(newBlock + BlockSize - sizeof(Slot_) + 1);
}

#include <iostream>
#include <cassert>
#include <time.h>
#include <vector>
#include <stack>

using namespace std;

/* Adjust these values depending on how much you trust your computer */
#define ELEMS 1000000
#define REPS 50

int main()
{

    clock_t start;

    MemoryPool<size_t> pool;
    start = clock();
    for (int i = 0; i < REPS; ++i)
    {
        for (int j = 0; j < ELEMS; ++j)
        {
            // 创建元素
            size_t *x = pool.newElement();

            // 释放元素
            pool.deleteElement(x);
        }
    }
    std::cout << "MemoryPool Time: ";
    std::cout << (((double)clock() - start) / CLOCKS_PER_SEC) << "\n\n";

    start = clock();
    for (int i = 0; i < REPS; ++i)
    {
        for (int j = 0; j < ELEMS; ++j)
        {
            size_t *x = new size_t;

            delete x;
        }
    }
    std::cout << "new/delete Time: ";
    std::cout << (((double)clock() - start) / CLOCKS_PER_SEC) << "\n\n";

    return 0;
}

#endif