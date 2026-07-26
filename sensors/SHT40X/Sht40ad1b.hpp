#pragma once

#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstring>

#include "FloatIntExtraction.hpp"
#include "Result.hpp"
#include "RingBuffer.hpp"
#include "cpp/II2C.hpp"
#include "crc_calculation.hpp"
#include "drivers.hpp"
#include "logger.hpp"

class Sht40ad1b
{
      private:
	enum class SensorState : uint8_t {
		IDLE,
		MEASURING,
		WAIT_DATA,
		DATA_READY
	};

      public:
	struct SensorData {
		float_t temperature{0};
		float_t humidity{0};
	};

      public:
	explicit Sht40ad1b(I2C_Ref mBus);

	// Disable copying to prevent memory / state duplication issues
	Sht40ad1b(const Sht40ad1b &) = delete;
	Sht40ad1b &operator=(const Sht40ad1b &) = delete;

	enum class Command : uint8_t {
		HIGH_PRECISION = 0XFDU,
		MEDIUM_PRECISION = 0XF6U,
		LOW_PRECISION = 0XE0U,
		INVALID = 0xFF,
	};

	// Public API
	bool initialize(Command cmd);

	bool isBusy() const;

	bool read();

	void ProcessData();

	SensorData getValue() const;

	SensorState getState() const;

	void onDataReceived();

	// Fault reporting
	Result<Unit, Err> getFaultStatus() const;
	void clearFault();

      private:
	void setState(SensorState state);
	void noteFailure(Err err);
	void noteSuccess();

      private:
	I2C_Ref hi2c;
	SensorState m_State{SensorState::IDLE};
	bool m_init{false};
	Command cmd{Command::INVALID};

	uint32_t measure_start_time = 0;
	static constexpr uint8_t DevAddr = 0x89U;
	static constexpr uint8_t kTimeOut = 10;
	static constexpr uint8_t kMaxRetries = 3;

	uint8_t m_retryCount{0};
	Err m_lastError{Err::None};

	/* Read Instruction Variables */
	SensorData m_data;
	std::array<uint8_t, 6> raw_data;
};
