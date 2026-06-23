#pragma once

#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstring>

#include "FloatIntExtraction.hpp"
#include "RingBuffer.hpp"
#include "cpp/II2C.hpp"
#include "crc_calculation.hpp"
#include "drivers.hpp"
#include "logger.hpp"


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