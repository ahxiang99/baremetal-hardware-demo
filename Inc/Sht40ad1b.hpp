#pragma once

#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstring>

#include "FloatIntExtraction.hpp"
#include "RingBuffer.hpp"
#include "cpp/II2C.hpp"
#include "cpp/systick.hpp"
#include "drivers.hpp"
#include "logger.hpp"

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

class Sht40ad1b {
   public:
    enum class SensorState : uint8_t { IDLE, MEASURING, WAIT_DATA, DATA_READY };

    struct SensorData {
        float_t temperature{0.0f};
        float_t humidity{0.0f};
    };

    Sht40ad1b(I2C_Ref mBus, const char* name) : hi2c(mBus) {
        std::strncpy(m_Name.data(), name, m_Name.size() - 1);
        m_Name.back() = '\0';
    }

    void read() {
        if (m_State == SensorState::IDLE) {
            if (hi2c.Write(DevAddr, &cmd, 1, 3)) {
                m_State            = SensorState::MEASURING;
                measure_start_time = getDrivers().my_systick.get_ticks();
            }
        }
    }

    void ProcessData() {
        if (m_State == SensorState::MEASURING) {
            if ((getDrivers().my_systick.get_ticks() - measure_start_time) > 30) {
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
        } else if (m_State == SensorState::WAIT_DATA) {
            if (getDrivers().my_systick.get_ticks() - measure_start_time > 50) {
                m_State = SensorState::IDLE;
            }
        }
    }

    SensorData getValue() const {
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

    void onDataReceived() {
        if (m_State == SensorState::WAIT_DATA) {
            setState(SensorState::DATA_READY);
        }
    }

   private:
    I2C_Ref                  hi2c;
    std::array<char, 32>     m_Name;
    SensorState              m_State{SensorState::IDLE};
    uint32_t                 measure_start_time = 0;

    static constexpr uint8_t DevAddr            = 0x89U;
    static constexpr uint8_t cmd                = 0xFDU;

    /* Read Instruction Variables */
    SensorData             m_data;
    std::array<uint8_t, 6> raw_data;
};

template <typename T>
class Packet {
    std::array<uint8_t, sizeof(T) + 4> m_bytes;

   public:
    explicit Packet(const T& data) {
        m_bytes[0] = 0xAAU;
        m_bytes[1] = 0x55U;
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

enum class PacketType {
    VERSION_1 = 0x01U,
    VERSION_2 = 0x02U,
};

template <typename T, PacketType pType>
class PacketV2 {
    std::array<uint8_t, sizeof(T) + 5> m_bytes;

   public:
    explicit PacketV2(const T& data) {
        m_bytes[0] = 0xAAU;
        m_bytes[1] = 0x55U;
        m_bytes[2] = static_cast<uint8_t>(pType);  // 0x01U for Version 1 and 0x02U for Version 2
        m_bytes[3] = sizeof(T);
        std::memcpy(&m_bytes[4], &data, sizeof(T));
        m_bytes[4 + sizeof(T)] = crc_calculate(&m_bytes[4], sizeof(T));
    }

    const uint8_t* raw() const {
        return m_bytes.data();
    }

    size_t size() const {
        return m_bytes.size();
    }
};