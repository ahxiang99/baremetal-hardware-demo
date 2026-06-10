#pragma once

#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>

#include "FloatIntExtraction.hpp"
#include "RingBuffer.hpp"
#include "Sensor.hpp"
#include "cpp/II2C.hpp"
#include "cpp/systick.hpp"
#include "drivers.hpp"
#include "logger.hpp"

#define SENSOR_DATA_PRINT 0

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

struct Sht40_Sensordata {
    float_t temperature;
    float_t humidity;
};

class Sht40ad1b {
   private:
    II2C&                    hi2c;
    std::array<char, 32>     m_Name;
    Sht40_Sensordata         m_data;
    SensorState              m_State = SensorState::IDLE;
    MySysTick&               m_systick;

    uint32_t                 measure_start_time = 0;

    static constexpr uint8_t DevAddr            = 0x89U;
    static constexpr uint8_t cmd                = 0xFDU;

    /* Read Instruction Variables */
    std::array<uint8_t, 6> raw_data;

   public:
    Sht40ad1b(II2C& mBus, MySysTick& systick, const char* name) : hi2c(mBus), m_systick(systick) {
        std::strncpy(m_Name.data(), name, m_Name.size() - 1);
        m_Name.back() = '\0';
    }

    void read() {
        if (m_State == SensorState::IDLE) {
            if (hi2c.Write(DevAddr, &cmd, 1, 3)) {
                m_State            = SensorState::MEASURING;
                measure_start_time = m_systick.get_ticks();
            }
        }
    }

    void ProcessData() {
        if (m_State == SensorState::MEASURING) {
            if ((m_systick.get_ticks() - measure_start_time) > 30) {
                if (hi2c.Read(DevAddr, raw_data.data(), raw_data.size(), 3)) {
                    m_State = SensorState::WAIT_DATA;
                } else {
                    m_State = SensorState::IDLE;
                }
            }
        } else if (m_State == SensorState::DATA_READY) {
            uint16_t temp_value_raw = (raw_data[0] * 0x100U) + raw_data[1];
            uint8_t  temp_value_crc = raw_data[2];
            uint16_t rh_value_raw   = (raw_data[3] * 0x100U) + raw_data[4];
            uint8_t  rh_value_crc   = raw_data[5];
            if (crc_check(&raw_data[0], 2, temp_value_crc) != 0U) {
                m_data.temperature = -45.0f + (175.0f * (float_t)temp_value_raw / (float_t)0xFFFF);
            } else {
                m_data.temperature = 0.0f;
            }

            if (crc_check(&raw_data[3], 2, rh_value_crc) != 0U) {
                m_data.humidity = -6.0f + (125.0f * (float_t)rh_value_raw / (float_t)0xFFFF);
                if (m_data.humidity < 0.0f)
                    m_data.humidity = 0.0f;
                else if (m_data.humidity > 100.0f)
                    m_data.humidity = 100;
            } else {
                m_data.humidity = 0.0f;
            }
            m_State = SensorState::IDLE;
#if SENSOR_DATA_PRINT
            FloatIntExtraction t1 = convertInt(m_data.temperature);
            FloatIntExtraction t2 = convertInt(m_data.humidity);

            LOG_PRINT("Temp: {}.{}", t1.Integer, t1.Decimal);
            LOG_PRINT("Rh: {}.{}", t2.Integer, t2.Decimal);
#endif
        }
    }

    Sht40_Sensordata getValue() const {
        return m_data;
    }

    const char* getName() const {
        return m_Name.data();
    }

    SensorState getState() const {
        return m_State;
    }

    void setState(SensorState state) {
        m_State = state;
    }
};

template <typename T>
class Packet {
    // sizeof(T) + 4 bytes:
    // [0xAA][0x55][LEN][...data...][CRC8]
    std::array<uint8_t, sizeof(T) + 4> m_bytes;

   public:
    explicit Packet(const T& data) {
        m_bytes[0] = 0xAA;
        m_bytes[1] = 0x55;
        m_bytes[2] = sizeof(T);
        std::memcpy(&m_bytes[3], &data, sizeof(T));
        m_bytes[3 + sizeof(T)] = crc_calculate(&m_bytes[3], sizeof(T));
    }
    const uint8_t* raw() const {
        return m_bytes.data();
    }
    size_t size() const {
        return m_bytes.size();
    }
};