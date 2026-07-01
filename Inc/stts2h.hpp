#pragma once

#include <atomic>
#include <cmath>
#include <cstdint>

#include "RingBuffer.hpp"
#include "Sht40ad1b.hpp"
#include "drivers.hpp"
#include "logger.hpp"

class Stts2h {
   public:
    enum class SensorState { IDLE, WAIT_DATA, DATA_READY };

    Stts2h(const I2C_Ref& i2c) : hi2c(i2c) {}

    void initialize() {
        /* Enable BDU - Block Data Update */
        uint8_t MASK = 1 << 6 | 1 << 3 | 1 << 7;
        hi2c.MemWrite(devAddr, 0x04U, &MASK, 1, 3);
        /* Enable Automatic Address Increment */

        /* Put the component in Standby Mode */
    }

    void get_whoami() {
        if (m_State.load(std::memory_order_relaxed) == SensorState::IDLE) {
            // Single MEM_READ transaction: START→DevAddr(W)→regAddr→Sr→DevAddr(R)→byte→STOP
            if (hi2c.MemRead(devAddr, whoAmIReg, m_data.data(), 1, 0x10U)) {
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
        if (m_State.load(std::memory_order_relaxed) == SensorState::IDLE) {
            if (hi2c.MemRead(devAddr, tempReg, m_data.data(), 2, 0x10U)) {
                m_State.store(SensorState::WAIT_DATA, std::memory_order_relaxed);
                LOG_DEBUG("STTS2H Called Read");
                last_call = getDrivers().my_systick.get_ticks();
            }
        }
    }

    void processData() {
        if (getDrivers().my_systick.get_ticks() - last_call > 500) {
            m_State.store(SensorState::IDLE, std::memory_order_relaxed);
            return;
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
    static constexpr uint8_t devAddr   = 0x71U;
    static constexpr uint8_t whoAmIReg = 0x01U;
    static constexpr uint8_t tempReg   = 0x06U;
    I2C_Ref                  hi2c;
    std::array<uint8_t, 2>   m_data;
    float_t                  m_Temp{0.0f};
    std::atomic<SensorState> m_State{SensorState::IDLE};
    bool                     whoami_called{false};
    uint32_t                 last_call{0};
};