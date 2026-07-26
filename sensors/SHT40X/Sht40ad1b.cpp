#include "Sht40ad1b.hpp"
#include "drivers.hpp"

Sht40ad1b::Sht40ad1b(I2C_Ref mBus) : hi2c(mBus)
{
}

bool Sht40ad1b::initialize(Command c)
{
	if (c != Command::INVALID) {
		cmd = c;
		m_init = true;
		return m_init;
	} else {
		return m_init;
	}
}

bool Sht40ad1b::isBusy() const
{
	return (m_State != SensorState::IDLE) && m_init;
}

bool Sht40ad1b::read()
{
	if (cmd == Command::INVALID || !m_init || isBusy()) {
		return false;
	}

	uint8_t byte = static_cast<uint8_t>(cmd);
	if (hi2c.Write(DevAddr, &byte, sizeof(byte), kTimeOut)) {
		m_State = SensorState::MEASURING;
		measure_start_time = getDrivers().my_systick.get_ticks();
		noteSuccess();
		return true;
	} else {
		noteFailure(Err::HwFault);
		return false;
	}
}

void Sht40ad1b::ProcessData()
{
	if (m_State == SensorState::MEASURING) {
		if ((getDrivers().my_systick.get_ticks() - measure_start_time) > 30) {
			if (hi2c.Read(DevAddr, raw_data.data(), raw_data.size(), 3)) {
				m_State = SensorState::WAIT_DATA;
			} else {
				m_State = SensorState::IDLE;
				noteFailure(Err::HwFault);
			}
		}
	} else if (m_State == SensorState::DATA_READY) {
		uint16_t temp_value_raw = (raw_data[0] * 0x100U) + raw_data[1];
		uint8_t temp_value_crc = raw_data[2];
		uint16_t rh_value_raw = (raw_data[3] * 0x100U) + raw_data[4];
		uint8_t rh_value_crc = raw_data[5];
		if (crc_check(&raw_data[0], 2, temp_value_crc) != 0U) {
			m_data.temperature =
				-45.0f + (175.0f * (float_t)temp_value_raw / (float_t)0xFFFF);
		} else {
			LOG_WARN("SHT40: temperature CRC mismatch (got {}, raw={})",
				 (uint32_t)temp_value_crc, (uint32_t)temp_value_raw);
			m_data.temperature = 0.0f;
		}

		if (crc_check(&raw_data[3], 2, rh_value_crc) != 0U) {
			m_data.humidity =
				-6.0f + (125.0f * (float_t)rh_value_raw / (float_t)0xFFFF);
			if (m_data.humidity < 0.0f) {
				m_data.humidity = 0.0f;
			} else if (m_data.humidity > 100.0f) {
				m_data.humidity = 100;
			}
		} else {
			LOG_WARN("SHT40: humidity CRC mismatch (got {}, raw={})",
				 (uint32_t)rh_value_crc, (uint32_t)rh_value_raw);
			m_data.humidity = 0.0f;
		}
		m_State = SensorState::IDLE;
	} else if (m_State == SensorState::WAIT_DATA) {
		if (getDrivers().my_systick.get_ticks() - measure_start_time > 50) {
			m_State = SensorState::IDLE;
			noteFailure(Err::Timeout);
		}
	}
}

Result<Unit, Err> Sht40ad1b::getFaultStatus() const
{
	if (m_lastError == Err::None) {
		return Ok();
	}
	return Result<Unit, Err>::fail(m_lastError);
}

void Sht40ad1b::clearFault()
{
	m_lastError = Err::None;
	m_retryCount = 0;
}

void Sht40ad1b::noteFailure(Err err)
{
	if (m_retryCount <= kMaxRetries) {
		++m_retryCount;
	}
	if (m_retryCount > kMaxRetries && m_lastError != err) {
		m_lastError = err;
		LOG_ERROR("SHT40: sensor fault after {} consecutive failures (err={})",
			  (uint32_t)m_retryCount, (uint32_t)err);
	}
}

void Sht40ad1b::noteSuccess()
{
	m_retryCount = 0;
}
Sht40ad1b::SensorState Sht40ad1b::getState() const
{
	return m_State;
}
void Sht40ad1b::setState(SensorState state)
{
	m_State = state;
}
void Sht40ad1b::onDataReceived()
{
	if (m_State == SensorState::WAIT_DATA) {
		setState(SensorState::DATA_READY);
	}
}
Sht40ad1b::SensorData Sht40ad1b::getValue() const
{
	return m_data;
}
