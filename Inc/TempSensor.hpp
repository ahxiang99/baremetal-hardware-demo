#pragma once
#include <cmath>
#include <cstdint>

class II2CMaster;

enum class SensorState { IDLE, TRIGGERED, WAIT_FOR_DATA, DATA_READY };

class TempSensor {
   private:
    II2CMaster& m_Bus;
    uint8_t     dev_addr;
    uint8_t     data[6];
    SensorState m_State;
    uint32_t    m_LastCallTick;
    float_t     value;

   public:
    TempSensor(II2CMaster& p_Bus);
    float_t     GetTemp();
    void        StartConversation();
    void        Process();
    float_t     getValue();
    void        AssignDriver(II2CMaster& p_Bus);
    SensorState GetState() const;
};