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

class SHT40X
{
  public:
    enum class SensorState : uint8_t {
	IDLE,
	MEASURING,
	WAIT_DATA,
	DATA_READY
    };

    struct SensorData {
	float_t temperature{0};
	float_t humidity{0};
    };

    enum class Command : uint8_t {
	HIGH_PRECISION = 0XFDU,
	MEDIUM_PRECISION = 0XF6U,
	LOW_PRECISION = 0XE0U,
	INVALID = 0xFF,
    };

  public:
    explicit SHT40X(const I2C_Ref &mBus);
    // Disable copying to prevent memory / state duplication issues
    SHT40X(const SHT40X &) = delete;
    SHT40X &operator=(const SHT40X &) = delete;

    // Public API
    [[nodiscard]] Result<> initialize(Command cmd);
    [[nodiscard]] bool isBusy() const;
    [[nodiscard]] SensorData getValue() const;
    [[nodiscard]] SensorState getState() const;
    bool read();
    void onDataReceived();

    // Fault reporting
    [[nodiscard]] Result<Unit, Err> getFaultStatus() const;
    void clearFault();
    void processData();

  private:
    void noteFailure(Err err);
    void noteSuccess();

  private:
    I2C_Ref hi2c;
    volatile SensorState m_State;
    Command sensor_cmd;
    uint32_t last_call;

    uint8_t m_retryCount;
    Err m_lastError;

    /* Read Instruction Variables */
    SensorData m_data;
    std::array<uint8_t, 6> raw_data;

    static constexpr uint8_t kDevAddr = 0x89U;
    static constexpr uint8_t kTimeOut = 25;
    static constexpr uint8_t kMaxRetries = 3;
};
