#pragma once

#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <optional>

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

template <typename T, size_t SIZE>
class SpscRingBuffer {
    static_assert((SIZE & (SIZE - 1)) == 0, "SIZE must be power of two");
    std::array<T, SIZE> buffer_;
    std::atomic<size_t> head_{0};  // write index — ISR owns
    std::atomic<size_t> tail_{0};  // read index — main owns

   public:
    bool push(const T& val) {
        size_t head = head_.load(std::memory_order_relaxed);
        size_t tail = tail_.load(std::memory_order_acquire);
        if (head - tail == SIZE) return false;

        buffer_[head & (SIZE - 1)] = val;
        head_.store(head + 1, std::memory_order_release);
        return true;
    }

    bool pop(T& out) {
        size_t head = head_.load(std::memory_order_acquire);
        size_t tail = tail_.load(std::memory_order_relaxed);

        if (head - tail == 0) return false;
        out = buffer_[tail & (SIZE - 1)];
        tail_.store(tail + 1, std::memory_order_release);
        return true;
    }

    uintptr_t data_ptr() const {
        return reinterpret_cast<uintptr_t>(buffer_.data());
    }
};