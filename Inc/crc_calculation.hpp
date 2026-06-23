#pragma once
#include <cstdint>

inline uint8_t crc_calculate(const uint8_t* data, uint16_t count) {
    const uint8_t crc8_polynomial = 0x31;
    uint8_t       crc             = 0xFF;

    /* Calculate 8-bit checksum for given polynomial */
    for (uint16_t index = 0; index < count; index++) {
        crc ^= data[index];
        for (uint8_t crc_bit = 8U; crc_bit > 0U; crc_bit--) {
            crc = ((crc & 0x80U) != 0U) ? ((crc << 1) ^ crc8_polynomial) : (crc << 1);
        }
    }

    return crc;
}

inline uint8_t crc_check(const uint8_t* data, uint16_t count, uint8_t crc) {
    return (crc_calculate(data, count) == crc) ? 1U : 0U;
}