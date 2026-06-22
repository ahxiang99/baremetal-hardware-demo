#pragma once

#include <cstdint>

#include "../common/RegisterUtils.hpp"
#include "RingBuffer.hpp"
#include "low-level/uart_bitfields.h"
#include "low-level/uart_registers.h"
#include "low-level/uart_types.h"

enum class UartState : uint8_t { Reset, Ready, BusyTx, BusyRx, Error };

enum class UartError : uint8_t { None, Timeout, Framing, Overrun, Parity, InvalidConfig };

enum class UartInitError : uint8_t { NullPeripheral, InvalidBaudRate, InvalidConfig, InvalidPostInit, AlreadyInitialised };

enum class UartNum : uint8_t { USART_D1, USART_D2, USART_D6, Dev_Total };

enum class UartBaudRate : uint16_t { _9600 = 0x683, _115200 = 0x008B };

enum class UartComm : uint8_t { RX_ONLY, TX_ONLY, RX_TX };

enum class UartParity : uint8_t { NONE, EVEN, ODD };

enum class UartStopBit : uint8_t { USART_CR2_STOP_1 = 0x00U, USART_CR2_STOP_2 = 0x10U };

struct UartConfig {
    UartNum      dev_num;
    UartBaudRate baudRate;
    UartComm     comm;
    UartParity   parity;
    UartStopBit  stopbits;
};

class Stm32Uart {
   public:
    Stm32Uart();
    Stm32Uart(const UartConfig& config);
    bool initialize();
    void configure(const UartConfig& config);
    bool send(const uint8_t* data, size_t DataLength);
    bool receive(uint8_t* buffer, size_t DataLength);

   protected:
    USART_TypeDef* uart_;
    UartConfig     config_;
    UartState      tx_state_{UartState::Reset};
    UartState      rx_state_{UartState::Reset};
    UartError      error_{UartError::None};
    bool           m_Init = false;

    void           clearFlag();
    void           enableNVICInterrupt();

   private:
    constexpr static uint32_t Timeout = 0x100U;
    void                      enablePeripheral();
    void                      disablePeripheral();

    void                      enablePeripheralClock();
    void                      disablePeripheralClock();

    void                      configureBaudRate();
    void                      configureParity();
    void                      configureStopBits();
    void                      configureComm();
};