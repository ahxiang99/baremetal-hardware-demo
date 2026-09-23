#include "SHT40X.hpp"
#include "drivers.hpp"

SHT40X::SHT40X(const I2C_Ref &mBus) : hi2c(mBus), m_State(SensorState::IDLE), sensor_cmd(Command::HIGH_PRECISION), last_call(0), m_retryCount(0), m_lastError(Err::None), raw_data()
{
}

Result<> SHT40X::initialize(Command cmd)
{
    if (cmd != Command::INVALID) {
	sensor_cmd = cmd;
	return Ok();
    } else {
	return Fail(Err::InvalidParam);
    }
}

bool SHT40X::isBusy() const
{
    return (m_State != SensorState::IDLE);
}

bool SHT40X::read()
{
    if (sensor_cmd == Command::INVALID || isBusy()) {
	return false;
    }

    const auto byte = static_cast<uint8_t>(sensor_cmd);
    if (hi2c.Write(kDevAddr, &byte, sizeof(byte), kTimeOut)) {
	m_State = SensorState::MEASURING;
	last_call = getDrivers().my_systick.get_ticks();
	noteSuccess();
	return true;
    } else {
	noteFailure(Err::HwFault);
	return false;
    }
}

void SHT40X::processData()
{
    switch (m_State) {
    case SensorState::IDLE:
	break;
    case SensorState::MEASURING:
	if ((getDrivers().my_systick.get_ticks() - last_call) > kTimeOut) {
	    if (hi2c.Read(kDevAddr, raw_data.data(), raw_data.size(), kTimeOut)) {
		m_State = SensorState::WAIT_DATA;
	    } else {
		m_State = SensorState::IDLE;
		noteFailure(Err::HwFault);
	    }
	}
	break;
    case SensorState::WAIT_DATA:
	if (getDrivers().my_systick.get_ticks() - last_call > kTimeOut) {
	    m_State = SensorState::IDLE;
	    noteFailure(Err::Timeout);
	}
	break;
    case SensorState::DATA_READY:
	const uint16_t temp_value_raw = (raw_data[0] * 0x100U) + raw_data[1];
	const uint8_t temp_value_crc = raw_data[2];
	const uint16_t rh_value_raw = (raw_data[3] * 0x100U) + raw_data[4];
	const uint8_t rh_value_crc = raw_data[5];
	if (crc_check(&raw_data[0], 2, temp_value_crc) != 0U) {
	    m_data.temperature = -45.0f + (175.0f * static_cast<float_t>(temp_value_raw) / static_cast<float_t>(0xFFFF));
	} else {
	    LOG_WARN("SHT40: temperature CRC mismatch (got {}, raw={})", static_cast<uint32_t>(temp_value_crc), static_cast<uint32_t>(temp_value_raw));
	    m_data.temperature = 0.0f;
	}
	if (crc_check(&raw_data[3], 2, rh_value_crc) != 0U) {
	    m_data.humidity = -6.0f + (125.0f * static_cast<float_t>(rh_value_raw) / static_cast<float_t>(0xFFFF));
	    if (m_data.humidity < 0.0f) {
		m_data.humidity = 0.0f;
	    } else if (m_data.humidity > 100.0f) {
		m_data.humidity = 100;
	    }
	} else {
	    LOG_WARN("SHT40: humidity CRC mismatch (got {}, raw={})", static_cast<uint32_t>(rh_value_crc), static_cast<uint32_t>(rh_value_raw));
	    m_data.humidity = 0.0f;
	}
	m_State = SensorState::IDLE;
	break;
    }
}

Result<Unit, Err> SHT40X::getFaultStatus() const
{
    if (m_lastError == Err::None) {
	return Ok();
    }
    return Result<Unit, Err>::fail(m_lastError);
}

void SHT40X::clearFault()
{
    m_lastError = Err::None;
    m_retryCount = 0;
}

void SHT40X::noteFailure(Err err)
{
    if (m_retryCount <= kMaxRetries) {
	++m_retryCount;
    }
    if (m_retryCount > kMaxRetries && m_lastError != err) {
	m_lastError = err;
	LOG_ERROR("SHT40: sensor fault after {} consecutive failures (err={})", static_cast<uint32_t>(m_retryCount), static_cast<uint32_t>(err));
    }
}

void SHT40X::noteSuccess()
{
    m_retryCount = 0;
}
void SHT40X::onDataReceived()
{
    if (m_State == SensorState::WAIT_DATA) {
	m_State = SensorState::DATA_READY;
	processData();
    }
}
SHT40X::SensorData SHT40X::getValue() const
{
    return m_data;
}
SHT40X::SensorState SHT40X::getState() const
{
    return m_State;
}
