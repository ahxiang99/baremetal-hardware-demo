#pragma once

#include <cstddef>
#include <cstdint>

#include "../common/RegisterUtils.hpp"
#include "low-level/uart.h"

enum class UartState { Reset, Ready, BusyTx, BusyRx, Error };

enum class UartError { None, Timeout, Framing, Overrun, Parity, InvalidConfig };

enum class UartNum { USART_D1, USART_D2, USART_D6 };

enum class UartBaudRate { _9600 = 0x683, _115200 = 0x008B };

enum class UartComm { RX_ONLY, TX_ONLY, RX_TX };

enum class UartParity { NONE, EVEN, ODD };

enum class UartStopBit { STOP_1, STOP_2 };

/* Constant Variables */

struct UartConfig {
    UartNum      dev_num;
    UartBaudRate baudRate;
    UartComm     comm;
    UartParity   parity;
    UartStopBit  stopbits;
};

class IUart {
   public:
    virtual ~IUart()                                          = default;
    virtual bool initialize()                                 = 0;
    virtual bool send(const uint8_t* data, size_t DataLength) = 0;
    virtual bool receive(uint8_t* buffer, size_t length)      = 0;
};