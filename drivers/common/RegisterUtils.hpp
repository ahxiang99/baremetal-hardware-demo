#pragma once

#include <cstdint>

#include "low-level/nvic.h"

namespace RegisterUtils {
template <typename T1, typename T2>
inline void modify(volatile T1& reg, T2 clearMask, T2 setMask) {
    reg = (reg & ~clearMask) | setMask;
}
template <typename T1, typename T2>
inline void setBits(volatile T1& reg, T2 Mask) {
    reg |= Mask;
}
template <typename T1, typename T2>
inline void clearBits(volatile T1& reg, T2 Mask) {
    reg &= ~Mask;
}
}  // namespace RegisterUtils