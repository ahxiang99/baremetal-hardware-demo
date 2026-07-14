#pragma once

#include <cstdint>

#include "../common/RegisterUtils.hpp"
#include "RingBuffer.hpp"
#include "low-level/uart_bitfields.h"
#include "low-level/uart_registers.h"
#include "low-level/uart_types.h"

enum class UartState_t : uint8_t { Reset, Ready, BusyTx, BusyRx, Error };

enum class UartError_t : uint8_t { None, Timeout, Framing, Overrun, Parity };

enum class UartDevice_t : uint8_t { USART_D1, USART_D2, USART_D6, USART_COUNT };

enum class UartBaudRate_t : uint32_t { BR_9600 = 9600, BR_115200 = 115200, BR_460800 = 460800, BR_921600 = 921600 };

enum class UartComm_t : uint8_t { RX_ONLY, TX_ONLY, RX_TX };

enum class UartParity_t : uint8_t { NONE, EVEN, ODD };

enum class UartStopBit_t : uint8_t { USART_CR2_STOP_1 = 0x00U, USART_CR2_STOP_2 = 0x10U };

struct UartConfig {
    UartDevice_t   dev_num;
    UartBaudRate_t baudRate;
    UartComm_t     comm;
    UartParity_t   parity;
    UartStopBit_t  stopbits;
};

class Stm32Uart {
   public:
    Stm32Uart();
    Result<>       initialize(const UartConfig& config);
    bool           send(const uint8_t* data, size_t DataLength);
    bool           receive(uint8_t* buffer, size_t DataLength);
    USART_TypeDef* rawInstance() const;
    void           disable();

   protected:
    USART_TypeDef* uart_;
    UartState_t    tx_state_{UartState_t::Reset};
    UartState_t    rx_state_{UartState_t::Reset};
    UartError_t    error_{UartError_t::None};
    bool           m_Init = false;

    void           clearFlag();
    Result<>       enable_nvic(USART_TypeDef* uart);

   private:
    void setBaudRate(UartBaudRate_t baudrate);
    void setParity(UartParity_t parity);
    void setStopBits(UartStopBit_t stopbit);
    void setComm(UartComm_t comm);

   private:
    static constexpr uint32_t kTimeout = 10;
};