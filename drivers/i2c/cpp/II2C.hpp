#pragma once

#include <cstdint>

#include "cpp/systick.hpp"

extern MySysTick my_systick;

class II2C {
   public:
    virtual ~II2C()                                                                                = default;
    virtual bool initialize()                                                                      = 0;
    virtual bool Write(uint16_t DevAddress, const uint8_t* pData, uint16_t Size, uint32_t Timeout) = 0;
    virtual bool Read(uint16_t DevAddress, uint8_t* pData, uint16_t Size, uint32_t Timeout)        = 0;
    virtual void handleEVInterrupt()                                                               = 0;
    virtual void handleERInterrupt()                                                               = 0;
};