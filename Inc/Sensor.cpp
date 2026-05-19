#include "Sensor.hpp"

void Sensor::SetCommBus(II2CMaster* p_Bus) {
    m_pBus = p_Bus;
}

II2CMaster* Sensor::GetCommBus() const {
    return m_pBus;
}

void Sensor::SetDevAddr(uint8_t addr) {
    dev_addr = addr;
}

uint8_t Sensor::GetDevAddr() const {
    return dev_addr;
}

void Sensor::SetState(SensorState state) {
    m_State = state;
}

SensorState Sensor::GetState() const {
    return m_State;
}

void Sensor::SetInit(bool state) {
    m_Init = state;
}
bool Sensor::IsInit() {
    return m_Init;
}