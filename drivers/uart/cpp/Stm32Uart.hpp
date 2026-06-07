#pragma once

#include "RingBuffer.hpp"
#include "UartBase.hpp"

class Stm32Uart : public UartBase {
   public:
    Stm32Uart() {}
    Stm32Uart(USART_TypeDef* uart, const UartConfig& config) : uart_{uart}, config_{config} {}
    void         setVariable(USART_TypeDef* uart, const UartConfig& config);
    bool         initialize() override;
    bool         send(const uint8_t* data, size_t DataLength) override;
    bool         receive(uint8_t* buffer, size_t DataLength) override;

    virtual void handleInterrupt() = 0;
    virtual void handleTxDmaInterrupt() {};
    virtual void handleRxDmaInterrupt() {};

   protected:
    USART_TypeDef* uart_;
    UartConfig     config_;
    virtual void   onPostUartInit() {};

    void           clearFlag();
    virtual void   onTxInterrupt() = 0;
    virtual void   onRxInterrupt() = 0;
    virtual void   onIdleInterrupt() {};

   private:
    void enablePeripheral();
    void disablePeripheral();

    void enablePeripheralClock();
    void disablePeripheralClock();

    void configureBaudRate();
    void configureParity();
    void configureStopBits();
    void configureComm();
};