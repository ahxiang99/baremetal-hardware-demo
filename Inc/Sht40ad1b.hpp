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
#include "logger.hpp"

extern MySysTick my_systick;

class Sht40ad1b {
   private:
    II2C&                    hi2c;
    std::array<char, 32>     m_Name;
    float_t                  m_Temp{0.0f};
    float_t                  m_Rh{0.0f};
    SensorState              m_State            = SensorState::IDLE;

    uint32_t                 measure_start_time = 0;

    static constexpr uint8_t DevAddr            = 0x89U;
    static constexpr uint8_t cmd                = 0xFDU;

    /* Read Instruction Variables */
    std::array<uint8_t, 6> raw_data;

   public:
    Sht40ad1b(II2C& mBus, const char* name) : hi2c(mBus) {
        std::strncpy(m_Name.data(), name, strlen(name));
    }

    void read() {
        if (m_State == SensorState::IDLE) {
            if (hi2c.Write(DevAddr, &cmd, 1, 3)) {
                m_State            = SensorState::MEASURING;
                measure_start_time = my_systick.get_ticks();
            }
        }
    }

    void ProcessData() {
        if (m_State == SensorState::MEASURING) {
            if ((my_systick.get_ticks() - measure_start_time) > 30) {
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
                m_Temp = -45.0f + (175.0f * (float_t)temp_value_raw / (float_t)0xFFFF);
            } else {
                m_Temp = 0.0f;
            }

            if (crc_check(&raw_data[3], 2, rh_value_crc) != 0U) {
                m_Rh = -6.0f + (125.0f * (float_t)rh_value_raw / (float_t)0xFFFF);
                if (m_Rh < 0.0f)
                    m_Rh = 0.0f;
                else if (m_Rh > 100.0f)
                    m_Rh = 100;
            } else {
                m_Rh = 0.0f;
            }

            FloatIntExtraction t1 = convertInt(m_Temp);
            FloatIntExtraction t2 = convertInt(m_Rh);

            LOG_PRINT("Temp: {}.{}", t1.Integer, t1.Decimal);
            LOG_PRINT("Rh: {}.{}", t2.Integer, t2.Decimal);
            m_State = SensorState::IDLE;
        }
    }

    float_t getValue() const {
        return m_Temp;
    }

    const char* getName() const {
        return m_Name.data();
    }

    SensorState getState() const {
        return m_State;
    }

    void setState(SensorState&& state) {
        m_State = state;
    }
};