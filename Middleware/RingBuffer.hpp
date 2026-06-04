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