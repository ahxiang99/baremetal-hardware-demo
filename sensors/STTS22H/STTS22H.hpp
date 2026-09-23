#pragma once

#include <atomic>
#include <cmath>
#include "drivers.hpp"
#include "SHT40X.hpp"
#include "logger.hpp"

class STTS22H
{
  public:
    enum class SensorState : uint8_t {
	IDLE,
	WAIT_DATA,
	DATA_READY
    };

    enum class SensorMode : uint8_t {
	ODR_25HZ,
	ODR_50HZ,
	ODR_100HZ,
	ODR_200HZ,
	ONE_SHOT,
	ODR_01HZ,
	INVALID
    };

    explicit STTS22H(const I2C_Ref &i2c);

    // Disable copying to prevent memory / state duplication issues
    STTS22H(const STTS22H &) = delete;
    STTS22H &operator=(const STTS22H &) = delete;

    [[nodiscard]] Result<> initialize(const SensorMode &mode);
    void read();
    void onDataReceived();
    [[nodiscard]] float_t getTemp() const;

  private:
    [[nodiscard]] uint8_t get_whoami();
    void processData();

    static constexpr uint8_t kDevAddr = 0x71U;
    static constexpr uint8_t REG_WHOAMI = 0x01U;
    static constexpr uint8_t WHO_AM_I = 0xA0U;
    static constexpr uint8_t REG_CTRL = 0x04U;
    static constexpr uint8_t REG_TEMP = 0x06U;
    static constexpr uint8_t kTimeout = 10;

    /* Bit Masks */
    static constexpr uint8_t CTRL_ONE_SHOT = 1 << 0;
    static constexpr uint8_t CTRL_TIME_OUT_DIS = 1 << 1;
    static constexpr uint8_t CTRL_FREERUN = 1 << 2;
    static constexpr uint8_t CTRL_IF_AND_INC = 1 << 3;
    static constexpr uint8_t CTRL_AVG0_POS = 4;
    static constexpr uint8_t CTRL_AVG1 = 1 << 5;
    static constexpr uint8_t CTRL_BDU = 1 << 6;
    static constexpr uint8_t CTRL_LOW_ODR_START = 1 << 7;

    I2C_Ref hi2c;
    SensorMode config{SensorMode::INVALID};
    volatile SensorState m_State{SensorState::IDLE};
    uint8_t config_byte;
    std::array<uint8_t, 2> raw_data;
    float_t m_Temp{0.0f};
    uint32_t last_call{0};
};
