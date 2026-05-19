#include "SHT4X.hpp"

#include <math.h>
#include <time.h>

#include "Sensor.hpp"
#include "cpp/i2c.hpp"
#include "cpp/systick.hpp"
#include "logger.hpp"

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

SHT4X::SHT4X() {
    SetCommBus(nullptr);
    SetDevAddr(0x00U);
    SetState(SensorState::IDLE);
    m_LastCallTick = 0;
    m_Temp = 0.0f;
    m_Rh = 0.0f;
}

void  SHT4X::Init(II2CMaster* p_Bus, uint8_t addr) {
    if (p_Bus == nullptr) {
        SetInit(false);
        return;
    }

    SetCommBus(p_Bus);
    SetDevAddr(addr);
    SetState(SensorState::IDLE);
    SetInit(true);

    for (int i = 0; i < 6; i++) {
        data[i] = 0;
    }
}

float_t SHT4X::getTemp() const {
    return m_Temp;
}
void SHT4X::SetTemp(float_t temp) {
    m_Temp = temp;
}

float_t SHT4X::getRh() const {
    return m_Rh;
}
void SHT4X::SetRh(float_t rh) {
    m_Rh = rh;
}

float_t SHT4X::PollingData() {

    if (!IsInit()) return 0.0f;

    II2CMaster* m_Bus = GetCommBus();
    uint8_t dev_addr = GetDevAddr();

    if (m_Bus == nullptr) return 0.0f;

    if (m_Bus->GetState() == I2C_ERROR_NACK) {
        m_Bus->SetState(I2C_READY);
    }

    uint32_t timeout = 10000;
    while (!m_Bus->isReady()) {
        if (--timeout == 0) return 0.0f;  // Bus is stuck, exit gracefully
    }

    uint8_t cmd = 0xFD;
    m_Bus->Write(dev_addr, &cmd, 1);

    timer.delay_ms(250);

    timeout = 10000;
    while (!m_Bus->isReady()) {
        if (--timeout == 0) return 0.0f;  // Bus is stuck, exit gracefully
    }

    m_Bus->Read(dev_addr, &data[0], 6);

    timeout = 10000;
    while (!m_Bus->isReady()) {
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

void SHT4X::StartConversation() {

    II2CMaster* m_Bus = GetCommBus();
    uint8_t dev_addr = GetDevAddr();

    if (m_Bus->GetState() == I2C_ERROR_NACK) {
        m_Bus->SetState(I2C_READY);
        SetState(SensorState::IDLE);
        LOG_DEBUG("Sensor is NACK. Bus reset.");
        return;
    } 
    
    if (GetState() == SensorState::IDLE) {
        if (m_Bus->isReady()) {
            uint8_t cmd = 0xFD;
            m_Bus->Write(dev_addr, &cmd, 1);
            SetState(SensorState::TRIGGERED);
            m_LastCallTick = timer.get_ticks();
        }
    }
}

void SHT4X::Process() {
    II2CMaster* m_Bus = GetCommBus();
    uint8_t dev_addr = GetDevAddr();

    if (m_Bus->GetState() == I2C_ERROR_NACK) {
        m_Bus->SetState(I2C_READY);
        SetState(SensorState::IDLE); // Throw away this failed sample and try again next loop
        LOG_DEBUG("Sensor is NACK. Bus reset.");
        return;
    }

    if (GetState() == SensorState::TRIGGERED) {
        if ((timer.get_ticks() - m_LastCallTick) >= 30) {
            if (m_Bus->isReady()) {
                m_Bus->Read(dev_addr, &data[0], 6);
                SetState(SensorState::WAIT_FOR_DATA);
            }
        }
    } else if (GetState() == SensorState::WAIT_FOR_DATA) {
        if (m_Bus->isReady()) {
            SetState(SensorState::DATA_READY);
        }
    } else if (GetState() == SensorState::DATA_READY) {
        uint16_t temp_value_raw = (data[0] * 0x100U) + data[1];
        uint8_t  temp_value_crc = data[2];
        uint16_t rh_value_raw = (data[3] * 0x100U) + data[4];
        uint8_t  rh_value_crc = data[5];
        if (crc_check(&data[0], 2, temp_value_crc) != 0U) { 
            float_t temp = -45.0f + (175.0f * (float_t)temp_value_raw / (float_t)0xFFFF);
            SetTemp(temp);
        } else {
            SetTemp(0.0f);
        }

        if (crc_check(&data[3], 2, rh_value_crc) != 0U) { 
            float_t rh = -6.0f + (125.0f * (float_t)rh_value_raw / (float_t)0xFFFF);
            if (rh < 0.0f) rh = 0.0f;
            else if (rh > 100.0f)  rh = 100;
            SetRh(rh);
        } else {
            SetRh(0.0f);
        }
        SetState(SensorState::IDLE);
    }
}