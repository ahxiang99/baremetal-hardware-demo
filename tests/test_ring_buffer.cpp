#include <gtest/gtest.h>

#include "RingBuffer.hpp"

// Test 1: push one byte, pop it back — verify correct value
TEST(SpscRingBuffer, PushPopSingleByte) {
    SpscRingBuffer<uint8_t, 4> buf;
    uint8_t                    a = 0x08U;
    buf.push(a);
    uint8_t b{0};
    buf.pop(b);
    EXPECT_EQ(a, b);
}
// Test 2: fill buffer to capacity, next push returns false
TEST(SpscRingBuffer, PushReturnsFalseWhenFull) {
    SpscRingBuffer<uint8_t, 4> buf;
    for (size_t i = 0; i < 4; ++i) {
        buf.push(0x01);
    }
    EXPECT_FALSE(buf.push(0x01));
}

// Test 3: empty buffer, pop returns false
TEST(SpscRingBuffer, PopReturnsFalseWhenEmpty) {
    SpscRingBuffer<uint8_t, 4> buf;
    uint8_t                    a{0};
    EXPECT_FALSE(buf.pop(a));
}