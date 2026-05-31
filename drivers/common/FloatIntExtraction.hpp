#pragma once
#include <cstdint>

struct FloatIntExtraction {
    uint16_t Integer;
    uint16_t Decimal;
};

inline FloatIntExtraction convertInt(float_t value) {
    FloatIntExtraction result = {0, 0};
    result.Integer            = (uint16_t)value;
    result.Decimal            = (uint16_t)((value - (float_t)result.Integer) * 100.0f);
    return result;
}