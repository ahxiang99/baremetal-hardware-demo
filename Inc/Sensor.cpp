#include "Sensor.hpp"

void Sensor::SetState(SensorState state) {
    m_State = state;
}

SensorState Sensor::GetState() const {
    return m_State;
}

void Sensor::SetName(std::string_view str) {
    for (size_t i = 0; i < str.size(); ++i) {
        dev_name.at(i) = str.at(i);
    }
}
const char* Sensor::GetName() const {
    return dev_name.data();
}