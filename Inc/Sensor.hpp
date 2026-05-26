#pragma once
#include <array>
#include <cmath>
#include <cstdint>
#include <string_view>

enum class SensorState { IDLE, MEASURING, WAIT_DATA, DATA_READY, ERROR };

static inline uint8_t crc_calculate(const uint8_t* data, uint16_t count) {
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

static inline uint8_t crc_check(const uint8_t* data, uint16_t count, uint8_t crc) {
    return (crc_calculate(data, count) == crc) ? 1U : 0U;
}

class Sensor {
   protected:
    SensorState          m_State = SensorState::IDLE;
    bool                 m_Init  = false;
    std::array<char, 32> dev_name;

   public:
    void        SetState(SensorState state);
    SensorState GetState() const;

    void        SetName(std::string_view str);
    const char* GetName() const;

    virtual ~Sensor()           = default;
    virtual void StartRead()    = 0;
    virtual void StartRead_IT() = 0;
};