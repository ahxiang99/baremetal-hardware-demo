#pragma once

#include <array>
#include <cmath>
#include <cstdint>

#include "Sensor.hpp"

class II2CMaster;

class SHT4X : public Sensor {
   private:
    // Hardware Address
    II2CMaster* m_Bus;
    uint8_t     m_DevAddr;

    // Last record of tick for delay
    uint32_t m_LastCallTick;

    // Data Variables
    std::array<uint8_t, 6> raw_data;
    float_t                m_Temp;
    float_t                m_Rh;

    // Data Manipulation
    void ProcessData();
    void ClearData();
    void SetTemp(float_t temp);
    void SetRh(float_t rh);

   public:
    // Init the hardware Sensor
    SHT4X();
    SHT4X(II2CMaster& pBus, uint8_t dev_addr, const char* name);

    // Data Getter
    float_t getTemp() const;
    float_t getRh() const;

    // Sensor Interface
    void Init(II2CMaster& pBus, uint8_t dev_addr, const char* name);
    void StartRead() override;
    void StartRead_IT() override;
};