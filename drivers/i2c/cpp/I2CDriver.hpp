#pragma once

#include <atomic>
#include <cstdint>
enum class I2C1_State : uint8_t { Idle, Busy, Done, Error };

class I2CDriver {
    std::atomic<I2C1_State> m_state{I2C1_State::Idle};
    SpscRingBuffer<16>      m_buffer;

   public:
    void startReceive() {
        if (m_state.load(std::memory_order_relaxed) == I2C1_State::Idle) {
            m_state.store(I2C1_State::Busy, std::memory_order_relaxed);
        }
    }
    bool isReady() {
        auto state = m_state.load(std::memory_order_acquire);
        return state == I2C1_State::Error || state == I2C1_State::Done;
    }

    // ISR callback — called by I2C1_EV_IRQHandler
    void onInterrupt(uint8_t received) {
        m_buffer.push(received);
        m_state.store(I2C1_State::Done, std::memory_order_release);
    }

    bool reset() {
        if (!m_buffer.isEmpty()) {
            m_state.store(I2C1_State::Error, std::memory_order::relaxed);
            return false;
        }
        m_state.store(I2C1_State::Idle, std::memory_order_relaxed);
        return true;
    }

    bool pop(uint8_t& out) {
        return m_buffer.pop(out);
    }
    I2C1_State getState() {
        return m_state.load(std::memory_order_relaxed);  // just checking, no sync needed
    }
};
