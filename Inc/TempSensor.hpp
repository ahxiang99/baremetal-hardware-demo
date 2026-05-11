#pragma once
#include <cmath>
#include <cstdint>

class II2CMaster;

class TempSensor {
   private:
    II2CMaster& m_Bus;
    uint8_t     dev_addr;
    uint8_t     data[6];

   public:
    TempSensor(II2CMaster& p_Bus);
    float_t GetTemp();
};