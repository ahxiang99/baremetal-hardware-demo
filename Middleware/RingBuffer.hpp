#pragma once

#include <array>
#include <cmath>
#include <optional>

#define BUF_SIZE 128

template <typename T>
class RingBuffer {
   private:
    size_t                  head = 0;
    size_t                  tail = 0;
    bool                    full = false;
    std::array<T, BUF_SIZE> buf_data;

   public:
    void push(T item) {
        buf_data[head] = item;
        if (full) {
            tail = (tail + 1) % BUF_SIZE;
        }
        head = (head + 1) % BUF_SIZE;
        full = (head == tail);
    }

    std::optional<T> pop() {
        if (empty()) return std::nullopt;
        T item = buf_data[tail];
        full   = false;
        tail   = (tail + 1) % BUF_SIZE;
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
};