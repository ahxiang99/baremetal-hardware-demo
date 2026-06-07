#include "SHT4X.hpp"

#include <math.h>
#include <time.h>

#include <type_traits>

#include "Sensor.hpp"
#include "bit_utils.h"
#include "cpp/systick.hpp"
#include "logger.hpp"
#include "low-level/i2c_bitfields.h"


extern MySysTick timer;

void             SHT4X::ClearData() {
    raw_data.fill(0);
}

void SHT4X::ProcessData() {
    uint16_t temp_value_raw = (raw_data[0] * 0x100U) + raw_data[1];
    uint8_t  temp_value_crc = raw_data[2];
    uint16_t rh_value_raw   = (raw_data[3] * 0x100U) + raw_data[4];
    uint8_t  rh_value_crc   = raw_data[5];
    if (crc_check(&raw_data[0], 2, temp_value_crc) != 0U) {
        float_t temp = -45.0f + (175.0f * (float_t)temp_value_raw / (float_t)0xFFFF);
        SetTemp(temp);
    } else {
        SetTemp(0.0f);
    }

    if (crc_check(&raw_data[3], 2, rh_value_crc) != 0U) {
        float_t rh = -6.0f + (125.0f * (float_t)rh_value_raw / (float_t)0xFFFF);
        if (rh < 0.0f)
            rh = 0.0f;
        else if (rh > 100.0f)
            rh = 100;
        SetRh(rh);
    } else {
        SetRh(0.0f);
    }
}

SHT4X::SHT4X() {
    // Clear all variables.
    SetState(SensorState::IDLE);
    m_Init = false;
    dev_name.fill(0);
    m_DevAddr      = 0x00;
    m_LastCallTick = 0;
    raw_data.fill(0);
}

SHT4X::SHT4X(II2CMaster& pBus, uint8_t dev_addr, const char* name) {
    Init(pBus, dev_addr, name);
}

void SHT4X::Init(II2CMaster& pBus, uint8_t dev_addr, const char* name) {
    SetState(SensorState::IDLE);
    SetName(name);
    m_Bus          = &pBus;
    m_DevAddr      = dev_addr;
    m_LastCallTick = 0;
    raw_data.fill(0);
    m_Init = true;
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

void SHT4X::StartRead() {
    if (!m_Init) return;

    if (GetState() == SensorState::ERROR) {
        SetState(SensorState::IDLE);
        return;
    }

    if (m_Bus->GetState() == I2C_ERROR_NACK) {
        m_Bus->SetState(I2C_READY);
    }

    SetState(SensorState::IDLE);

    uint32_t timeout = 10000;
    while (!m_Bus->isReady()) {
        if (--timeout == 0) {
            SetState(SensorState::ERROR);
            return;
        }
    }

    uint8_t cmd = 0xFD;
    m_Bus->Write(m_DevAddr, &cmd, 1);
    SetState(SensorState::MEASURING);
    timer.delay_ms(30);

    timeout = 10000;
    while (!m_Bus->isReady()) {
        if (--timeout == 0) {
            SetState(SensorState::ERROR);
            return;
        }  // Bus is stuck, exit gracefully  // Bus is stuck, exit gracefully
    }

    m_Bus->Read(m_DevAddr, &raw_data[0], 6);

    SetState(SensorState::WAIT_DATA);
    timeout = 10000;
    while (!m_Bus->isReady()) {
        if (--timeout == 0) {
            SetState(SensorState::ERROR);
            return;
        }  // Bus is stuck, exit gracefully
    }

    SetState(SensorState::DATA_READY);
    ProcessData();
}

void SHT4X::StartRead_IT() {
    if (m_Bus->GetState() == I2C_ERROR_NACK) {
        m_Bus->SetState(I2C_READY);
        SetState(SensorState::IDLE);
        LOG_DEBUG("{} is NACK. Bus reset.", GetName());
        return;
    }

    uint8_t cmd = 0xFD;
    switch (m_State) {
        case SensorState::IDLE:
            break;
        case SensorState::MEASURING:
            if (m_Bus->isReady()) {
                if (m_Bus->Write(m_DevAddr, &cmd, 1) == I2C_OK) {
                    m_LastCallTick = timer.get_ticks();
                    SetState(SensorState::WAIT_DATA);
                } else {
                    SetState(SensorState::ERROR);
                }
            }
            break;
        case SensorState::WAIT_DATA:
            if ((timer.get_ticks() - m_LastCallTick) >= 30) {
                if (m_Bus->isReady()) {
                    m_Bus->Read(m_DevAddr, raw_data.data(), 6);
                    SetState(SensorState::DATA_READY);
                }
            }
            break;
        case SensorState::DATA_READY:
            ProcessData();
            SetState(SensorState::IDLE);
            break;
        case SensorState::ERROR:
            break;

        default:
            break;
    }
}