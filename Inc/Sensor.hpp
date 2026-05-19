#pragma once
#include <cmath>
#include <cstdint>
class II2CMaster;

enum class SensorState { IDLE, TRIGGERED, WAIT_FOR_DATA, DATA_READY };

class Sensor {
    private:
        II2CMaster* m_pBus;
        uint8_t     dev_addr;
        SensorState m_State;
        bool        m_Init = false;

    
   public:
   void SetCommBus(II2CMaster* p_Bus);
   II2CMaster* GetCommBus() const;
   
   void SetDevAddr(uint8_t addr);
   uint8_t GetDevAddr() const;
   
   void SetState(SensorState state);
   SensorState GetState() const;
   
   void SetInit(bool state);
   bool IsInit();
   
   virtual ~Sensor()               = default;
   virtual void    Init(II2CMaster* p_Bus, uint8_t addr) = 0; // Initialize Sensor

   virtual void    StartConversation() = 0; // Start Capturing Data
   virtual void    Process()       = 0; // Start Listening from Bus to receive data and write to m_Value
};