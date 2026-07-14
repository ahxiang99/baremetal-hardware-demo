#pragma once

#include <array>
#include <cstdint>
#include <functional>

#include "Result.hpp"
#include "RingBuffer.hpp"
#include "SensorRef.hpp"
#include "Stm32I2C.hpp"
#include "pch.hpp"

class InterruptI2C : public Stm32I2C {
public:
  Result<> initialize(const i2c_config_t &cfg);
  bool Write(uint16_t DevAddress, uint8_t *pData, uint16_t Size,
             uint32_t Timeout);
  bool Read(uint16_t DevAddress, uint8_t *pData, uint16_t Size,
            uint32_t Timeout);
  void handleEVInterrupt();
  void handleERInterrupt();

  void processRx();
  void onDataReceived();

  template <Sensor T> void addReceiver(T &obj) {
    if (receivers_count < receivers_.size()) {
      receivers_[receivers_count++] = SensorRef(obj);
    }
  }

protected:
  uint8_t DevAddr;
  uint8_t *XferPtr;
  uint32_t XferSize;
  bool isHardwareBusy(const uint32_t &Timeout);
  void enable_interrupt();
  void disable_interrupt();

private:
  std::atomic_bool RxEventFlag{false};
  RingBuffer<uint8_t, CHUNK_SIZE> RxBuffer;

  static constexpr uint8_t SENSOR_COUNT = 8;
  std::array<SensorRef, SENSOR_COUNT> receivers_;
  uint8_t receivers_count{0};
};