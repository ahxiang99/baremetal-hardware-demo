#pragma once

#include <atomic>
#include <cmath>
#include <cstdint>

#include "RegisterUtils.hpp"
#include "RingBuffer.hpp"
#include "Sht40ad1b.hpp"
#include "drivers.hpp"
#include "logger.hpp"
#include "projdefs.h"

class Stts2h {
   public:
    enum class SensorState : uint8_t { IDLE, MEASURING, WAIT_DATA, DATA_READY };

    enum class SensorMode : uint8_t { FREE_RUN_25HZ = 0, FREE_RUN_50HZ, FREE_RUN_100HZ, FREE_RUN_200HZ, ONE_SHOT, LOW_ODR };

    Stts2h(const I2C_Ref& i2c, const SensorMode& mode) : hi2c(i2c), m_Mode(mode) {}

    void initialize() {
        /* Put the component in Standby Mode */
        m_Mask = BDU_MASK | INC_MASK;  // <- Enable BDU and Automatic Address Increment
        /* Configure Sensor Mode */
        switch (m_Mode) {
            case SensorMode::ONE_SHOT:
                RegisterUtils::clearBits(m_Mask, LOW_ODR_MASK);
                break;
            case SensorMode::LOW_ODR:
                RegisterUtils::setBits(m_Mask, LOW_ODR_MASK);
                break;
            case SensorMode::FREE_RUN_25HZ:
            case SensorMode::FREE_RUN_50HZ:
            case SensorMode::FREE_RUN_100HZ:
            case SensorMode::FREE_RUN_200HZ:
                RegisterUtils::setBits(m_Mask, FREERUN_MASK);
                RegisterUtils::clearBits(m_Mask, LOW_ODR_MASK);
                RegisterUtils::clearBits(m_Mask, CLEAR_AVG_0);
                RegisterUtils::setBits(m_Mask, static_cast<uint8_t>(static_cast<uint8_t>(m_Mode) << 4));
                if (m_Mode == SensorMode::FREE_RUN_25HZ) {
                    RegisterUtils::clearBits(m_Mask, CLEAR_AVG_0);
                }
                break;
        }

        hi2c.MemWrite(DEV_ADDR, CTRL_REG, (uint8_t*)&m_Mask, 1, TIMEOUT);
    }

    void get_whoami() {
        if (m_State.load(std::memory_order_relaxed) == SensorState::IDLE) {
            // Single MEM_READ transaction: START→DevAddr(W)→regAddr→Sr→DevAddr(R)→byte→STOP
            if (hi2c.MemRead(DEV_ADDR, whoAmIReg, m_data.data(), 1, TIMEOUT)) {
                m_State.store(SensorState::WAIT_DATA, std::memory_order_relaxed);
                LOG_DEBUG("Call Who Am I...");
                whoami_called = true;
            } else {
                LOG_DEBUG("Failed to Call Who Am I");
            }
        }
    }

    bool isIdle() const {
        return m_State == SensorState::IDLE;
    }

    void read() {
        if (m_Mode != SensorMode::ONE_SHOT) {
            if (m_State.load(std::memory_order_relaxed) == SensorState::IDLE) {
                if (hi2c.MemRead(DEV_ADDR, TEMP_REG, m_data.data(), 2, TIMEOUT)) {
                    m_State.store(SensorState::WAIT_DATA, std::memory_order_relaxed);
                    LOG_DEBUG("STTS2H Called Read");
                    last_call = getDrivers().my_systick.get_ticks();
                }
            }
        } else {
            /* One Shot Mode */
            if (m_State.load(std::memory_order_relaxed) == SensorState::IDLE) {
                m_Mask |= ONESHOT_MASK;
                if (hi2c.MemWrite(DEV_ADDR, CTRL_REG, (uint8_t*)&m_Mask, 1, TIMEOUT)) {
                    m_State.store(SensorState::MEASURING, std::memory_order_relaxed);
                    last_call = getDrivers().my_systick.get_ticks();
                }
            }
        }
    }

    void processData() {
        if (getDrivers().my_systick.get_ticks() - last_call > 500) {
            m_State.store(SensorState::IDLE, std::memory_order_relaxed);
            return;
        }

        if (m_Mode == SensorMode::ONE_SHOT) {
            if ((m_State.load(std::memory_order_relaxed) == SensorState::MEASURING) && (getDrivers().my_systick.get_ticks() - last_call > TIMEOUT)) {
                if (hi2c.MemRead(DEV_ADDR, TEMP_REG, m_data.data(), 2, TIMEOUT)) {
                    m_State.store(SensorState::WAIT_DATA, std::memory_order_relaxed);
                    last_call = getDrivers().my_systick.get_ticks();
                }
            }
        }

        if (m_State.load(std::memory_order_acquire) == SensorState::DATA_READY) {
            if (whoami_called) {
                LOG_DEBUG("Stts2h Who Am I: {}", (Hex)m_data[0]);
                whoami_called = false;
            }

            int16_t temp_ticks = (int16_t)m_data[1];
            temp_ticks         = (temp_ticks * 256) + (int16_t)m_data[0];
            m_Temp             = ((float_t)temp_ticks / 100.0f);
            m_State.store(SensorState::IDLE, std::memory_order_relaxed);
        }
    }

    float_t getTemp() const {
        return m_Temp;
    }

    SensorState getState() const {
        return m_State.load(std::memory_order_relaxed);
    }

    void setState(SensorState s) {
        m_State.store(s, std::memory_order_release);
    }

    void onDataReceived() {
        if (m_State == SensorState::WAIT_DATA) {
            setState(SensorState::DATA_READY);
        }
    }

   private:
    static constexpr uint8_t DEV_ADDR        = 0x71U;
    static constexpr uint8_t whoAmIReg       = 0x01U;
    static constexpr uint8_t CTRL_REG        = 0x04U;
    static constexpr uint8_t STATUS_REG      = 0x05U;
    static constexpr uint8_t TEMP_REG        = 0x06U;
    static constexpr uint8_t ONESHOT_MASK    = 1 << 0;
    static constexpr uint8_t FREERUN_MASK    = 1 << 2;
    static constexpr uint8_t INC_MASK        = 1 << 3;
    static constexpr uint8_t BDU_MASK        = 1 << 6;
    static constexpr uint8_t LOW_ODR_MASK    = 1 << 7;
    static constexpr uint8_t CLEAR_AVG_0     = 3 << 4;
    static constexpr uint8_t TIMEOUT         = 3U;
    static constexpr uint8_t CONVERSION_TIME = 3U;

    I2C_Ref                  hi2c;
    std::array<uint8_t, 2>   m_data;
    float_t                  m_Temp{0.0f};
    std::atomic<SensorState> m_State{SensorState::IDLE};
    SensorMode               m_Mode;
    uint32_t                 m_Mask;  // For One Shot Mode
    bool                     whoami_called{false};
    uint32_t                 last_call{0};
};