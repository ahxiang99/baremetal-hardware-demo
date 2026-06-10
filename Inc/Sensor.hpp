#pragma once
#include <array>
#include <cmath>
#include <cstdint>
#include <string_view>

enum class SensorState { IDLE, MEASURING, WAIT_DATA, DATA_READY, ERROR };

class Sensor {
   protected:
    SensorState          m_State = SensorState::IDLE;
    bool                 m_Init  = false;
    std::array<char, 32> dev_name;

   public:
    void        SetState(SensorState state);
    SensorState GetState() const;

    void        SetName(std::string_view str);
    const char* GetName() const;

    virtual ~Sensor()           = default;
    virtual void StartRead()    = 0;
    virtual void StartRead_IT() = 0;
};
