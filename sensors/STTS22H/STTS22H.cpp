#include "STTS22H.hpp"
#include "pch.hpp"

STTS22H::STTS22H(const I2C_Ref &i2c) : hi2c(i2c), config_byte(0), raw_data{0}
{
}

Result<> STTS22H::initialize(const SensorMode &mode)
{
    /* Enable BDU - Block Data Update, Enable Automatic Address Increment */
    config_byte = CTRL_BDU | CTRL_IF_AND_INC;
    switch (mode) {
    case SensorMode::ONE_SHOT:
	break;
    case SensorMode::ODR_01HZ:
	config_byte |= CTRL_LOW_ODR_START;
	break;
    case SensorMode::ODR_25HZ:
    case SensorMode::ODR_50HZ:
    case SensorMode::ODR_100HZ:
    case SensorMode::ODR_200HZ:
	config_byte |= CTRL_FREERUN | static_cast<uint8_t>(mode) << CTRL_AVG0_POS;
	break;
    case SensorMode::INVALID:
	break;
    }
    
    if (get_whoami() == WHO_AM_I) {
	if (hi2c.MemWrite(kDevAddr, REG_CTRL, &config_byte, sizeof(config_byte), kTimeout)) {
	    return Ok();
	} else {
	    return Fail(Err::NotInitialized);
	}
    } else {
	return Fail(Err::HwFault);
    }
}
uint8_t STTS22H::get_whoami()
{
    // Single MEM_READ transaction: START→DevAddr(W)→regAddr→Sr→DevAddr(R)→byte→STOP
    uint8_t received_byte = 0;
    if (hi2c.MemRead(kDevAddr, REG_WHOAMI, &received_byte, sizeof(received_byte), kTimeout)) {
	m_State = SensorState::WAIT_DATA;
	last_call = getDrivers().my_systick.get_ticks();
	while ((m_State != SensorState::DATA_READY) && (getDrivers().my_systick.get_ticks() - last_call < kTimeout)) {
	    hi2c.processRx();
	}
    }
    return received_byte;
}

void STTS22H::read()
{
    if (m_State == SensorState::IDLE) {
	if (hi2c.MemRead(kDevAddr, REG_TEMP, raw_data.data(), raw_data.size(), kTimeout)) {
	    m_State = SensorState::WAIT_DATA;
	    last_call = getDrivers().my_systick.get_ticks();
	}
    }
}
void STTS22H::processData()
{
    if (getDrivers().my_systick.get_ticks() - last_call > 500) {
	m_State = SensorState::IDLE;
	return;
    }

    if (m_State == SensorState::DATA_READY) {
	const auto temp_ticks = static_cast<int16_t>(raw_data[1]) << 8 | raw_data[0];
	m_Temp = static_cast<float_t>(temp_ticks) / 100.0f;
	m_State = SensorState::IDLE;
    }
}
float_t STTS22H::getTemp() const
{
    return m_Temp;
}
void STTS22H::onDataReceived()
{
    if (m_State == SensorState::WAIT_DATA) {
	m_State = SensorState::DATA_READY;
	processData();
    }
}
