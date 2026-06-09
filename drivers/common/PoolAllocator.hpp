#pragma once

#include <cmath>
#include <cstdint>

template <size_t BlockSize, size_t BlockCount>
class PoolAllocator {
    static_assert(BlockSize >= sizeof(void*), "It must at least 4 bytes.");
    struct FreeBlock {
        FreeBlock* next;
    };

    FreeBlock* head = nullptr;
    uint8_t    storage[BlockCount][BlockSize]{0};

   public:
    PoolAllocator() {
        for (size_t i = 0; i < BlockCount - 1; ++i) {
            reinterpret_cast<FreeBlock*>(storage[i])->next = reinterpret_cast<FreeBlock*>(storage[i + 1]);
        }
        reinterpret_cast<FreeBlock*>(storage[BlockCount - 1])->next = nullptr;
        head                                                        = reinterpret_cast<FreeBlock*>(storage[0]);
    }
    void* allocate() {
        if (head == nullptr) {
            return nullptr;
        }
        void* temp = head;
        head       = head->next;
        return temp;
    }
    void deallocate(void* ptr) {
        reinterpret_cast<FreeBlock*>(ptr)->next = head;
        head                                    = reinterpret_cast<FreeBlock*>(ptr);
    }
};

template <size_t BlockSize, size_t BlockCount>
class PoolPtr {
    PoolAllocator<BlockSize, BlockCount>& m_pool;
    void*                                 m_ptr;

   public:
    explicit PoolPtr(PoolAllocator<BlockSize, BlockCount>& pool) : m_pool(pool), m_ptr(pool.allocate()) {}

    ~PoolPtr() {
        if (m_ptr) {
            m_pool.deallocate(m_ptr);
        }
    }

    PoolPtr(PoolPtr&& others) : m_pool(others.m_pool), m_ptr(others.m_ptr) {
        others.m_ptr = nullptr;
    }

    PoolPtr& operator=(PoolPtr&& others) {
        if (this == &others) return *this;

        if (m_ptr) {
            m_pool.deallocate(m_ptr);
        }
        m_ptr        = others.m_ptr;
        others.m_ptr = nullptr;
        return *this;
    }

    PoolPtr(const PoolPtr&)            = delete;

    PoolPtr& operator=(const PoolPtr&) = delete;

    uint8_t& operator*() {
        return *reinterpret_cast<uint8_t*>(m_ptr);
    }

    uint8_t* operator->() {
        return reinterpret_cast<uint8_t*>(m_ptr);
    }

    void* get() const {
        return m_ptr;
    }
};