#pragma once

#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <optional>

template <size_t SIZE>
class SpscRingBuffer {
    static_assert((SIZE > 0) && ((SIZE & (SIZE - 1)) == 0), "SIZE must be power of two");

    uint8_t             m_buffer[SIZE];
    std::atomic<size_t> m_head{0};  // ISR writes
    std::atomic<size_t> m_tail{0};  // main writes
    std::atomic<bool>   full{false};

   public:
    // ISR — push one byte, drop if full, set error flag
    bool push(uint8_t byte) {
        if (!isFull()) {
            auto head      = m_head.load(std::memory_order_relaxed);
            m_buffer[head] = byte;
            head           = (head + 1) & (SIZE - 1);
            auto tail      = m_tail.load(std::memory_order_relaxed);
            full.store(head == tail, std::memory_order_relaxed);
            m_head.store(head, std::memory_order_release);
            return true;
        } else {
            /* Unable to Push */
            return false;
        }
    }

    // main — pop one byte, return false if empty
    bool pop(uint8_t& out) {
        if (!isEmpty()) {
            auto tail = m_tail.load(std::memory_order_relaxed);
            out       = m_buffer[tail];
            tail      = (tail + 1) & (SIZE - 1);
            full.store(false, std::memory_order_relaxed);
            m_tail.store(tail, std::memory_order_release);
            return true;
        } else {
            return false;
        }
    }

    // helpers
    bool isEmpty() {
        return (!full.load(std::memory_order_relaxed) && m_head.load(std::memory_order_acquire) == m_tail.load(std::memory_order_relaxed));
    }
    bool isFull() {
        return full.load(std::memory_order_relaxed);
    }
};

template <typename T, size_t BUF_SIZE>
class RingBuffer {
   private:
    volatile size_t         head = 0;
    volatile size_t         tail = 0;
    bool                    full = false;
    std::array<T, BUF_SIZE> buf_data;
    static_assert((BUF_SIZE > 0) && ((BUF_SIZE & (BUF_SIZE - 1)) == 0), "Buffer Size must be power of 2.");

   public:
    void push(T item) {
        buf_data[head] = item;
        if (full) {
            tail = (tail + 1) & (BUF_SIZE - 1);
        }
        head = (head + 1) & (BUF_SIZE - 1);
        full = (head == tail);
    }

    std::optional<T> pop() {
        if (empty()) return std::nullopt;
        T item = buf_data[tail];
        full   = false;
        tail   = (tail + 1) & (BUF_SIZE - 1);
        return item;
    }

    bool empty() const {
        return (!full && (head == tail));
    }

    bool is_full() const {
        return full;
    }

    size_t size() const {
        if (full) return BUF_SIZE;
        if (head >= tail) return head - tail;
        return BUF_SIZE + head - tail;
    }

    uintptr_t data_ptr() const {
        return reinterpret_cast<uintptr_t>(buf_data.data());
    }

    void sync_dma_head(size_t dma_ndtr) {
        head = BUF_SIZE - dma_ndtr;
        full = false;
    }

    void remove_last() {
        head = head - 1;
    }
};