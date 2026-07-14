#pragma once

#include <cstdint>

namespace RegisterUtils {
inline void modify(volatile uint32_t& reg, uint32_t clearMask, uint32_t setMask) {
    reg = (reg & ~clearMask) | setMask;
}

inline void setBits(volatile uint32_t& reg, uint32_t Mask) {
    reg |= Mask;
}

inline void clearBits(volatile uint32_t& reg, uint32_t Mask) {
    reg &= ~Mask;
}
}  // namespace RegisterUtils