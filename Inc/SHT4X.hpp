#pragma once
#include <cmath>
#include <cstdint>
#include "Sensor.hpp"

class II2CMaster;

class SHT4X : public Sensor {
   private:
    uint32_t    m_LastCallTick;
    float_t     m_Temp;
    float_t     m_Rh;
    uint8_t     data[6];

   public:
    SHT4X();
    float_t     PollingData(); // Polling Method.

    float_t getTemp() const;
    void SetTemp(float_t temp);

    float_t getRh() const;
    void SetRh(float_t rh);

    void        Init(II2CMaster* p_Bus, uint8_t addr) override;
    void        StartConversation() override;
    void        Process() override;
};