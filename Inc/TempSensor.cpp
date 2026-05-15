#include "TempSensor.hpp"

#include <math.h>
#include <time.h>

#include "cpp/i2c.hpp"
#include "cpp/systick.hpp"

extern MySysTick timer;

uint8_t          crc_calculate(const uint8_t* data, uint16_t count) {
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

uint8_t crc_check(const uint8_t* data, uint16_t count, uint8_t crc) {
    return (crc_calculate(data, count) == crc) ? 1U : 0U;
}

TempSensor::TempSensor(II2CMaster& p_Bus) : m_Bus(p_Bus), dev_addr(0x88), m_State{SensorState::IDLE} {}

float_t TempSensor::GetTemp() {
    if (m_Bus.GetState() == I2C_ERROR_NACK) {
        m_Bus.SetState(I2C_READY);
    }

    uint32_t timeout = 10000;
    while (!m_Bus.isReady()) {
        if (--timeout == 0) return 0.0f;  // Bus is stuck, exit gracefully
    }

    uint8_t cmd = 0xFD;
    m_Bus.Write(dev_addr, &cmd, 1);

    timer.delay_ms(250);

    timeout = 10000;
    while (!m_Bus.isReady()) {
        if (--timeout == 0) return 0.0f;  // Bus is stuck, exit gracefully
    }

    m_Bus.Read(dev_addr, &data[0], 6);

    timeout = 10000;
    while (!m_Bus.isReady()) {
        if (--timeout == 0) return 0.0f;  // Bus is stuck, exit gracefully
    }

    uint16_t temp_value_raw = (data[0] * 0x100U) + data[1];
    uint8_t  temp_value_crc = data[2];
    if (crc_check(&data[0], 2, temp_value_crc) != 0U) {
        float_t temperature = -45.0f + (175.0f * (float_t)temp_value_raw / (float_t)0xFFFF);
        return temperature;
    } else {
        return 0.0f;
    }
}

void TempSensor::StartConversation() {
    if (m_Bus.GetState() == I2C_ERROR_NACK) {
        m_Bus.SetState(I2C_READY);
    }
    if (m_Bus.isReady() && m_State == SensorState::IDLE) {
        uint8_t cmd = 0xFD;
        m_Bus.Write(dev_addr, &cmd, 1);
        m_State        = SensorState::TRIGGERED;
        m_LastCallTick = timer.get_ticks();
    }
}

void TempSensor::Process() {
    if (m_State == SensorState::TRIGGERED && (timer.get_ticks() - m_LastCallTick) >= 25) {
        if (m_Bus.isReady()) {
            m_Bus.Read(dev_addr, &data[0], 6);
            m_State = SensorState::DATA_READY;
        }
    }

    if (m_State == SensorState::DATA_READY && m_Bus.isReady()) {
        uint16_t temp_value_raw = (data[0] * 0x100U) + data[1];
        uint8_t  temp_value_crc = data[2];
        if (crc_check(&data[0], 2, temp_value_crc) != 0U) {
            value = -45.0f + (175.0f * (float_t)temp_value_raw / (float_t)0xFFFF);
        } else {
            value = 0.0f;
        }
        m_State = SensorState::IDLE;
    }
}

SensorState TempSensor::GetState() const {
    return m_State;
}

float_t TempSensor::getValue() {
    return value;
}

void TempSensor::AssignDriver(II2CMaster& p_Bus) {
    m_Bus = p_Bus;
}