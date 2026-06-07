#include "Stm32Uart.hpp"

#include <cstdint>
#include <set>

#include "IUart.hpp"
#include "RegisterUtils.hpp"
#include "low-level/nvic.h"
#include "low-level/rcc.h"
#include "low-level/uart_types.h"

bool Stm32Uart::initialize() {
    enablePeripheralClock();
    disablePeripheral();

    configureComm();
    configureBaudRate();
    configureParity();
    configureStopBits();

    enablePeripheral();
    onPostUartInit();

    My_NVIC_EnableIRQ(38);
    return true;
}

void Stm32Uart::configureBaudRate() {
    uart_->BRR = static_cast<uint32_t>(config_.baudRate);
}

void Stm32Uart::configureParity() {
    // Explicit Bit Cwnership
    constexpr uint32_t Mask = USART_CR1_PCE | USART_CR1_PS;
    uint32_t           temp = uart_->CR1;
    // Only Parity Bit are modified
    RegisterUtils::clearBits(temp, Mask);

    switch (config_.parity) {
        case UartParity::NONE:
            // Disable Parity Control Enable
            break;
        case UartParity::EVEN:
            RegisterUtils::setBits(temp, USART_CR1_PCE);
            break;
        case UartParity::ODD:
            RegisterUtils::setBits(temp, Mask);
            break;
        default:
            SetError(UartError::InvalidConfig);
            return;
    }
    uart_->CR1 = temp;
}

void Stm32Uart::configureStopBits() {
    constexpr uint32_t Mask = USART_CR2_STOP;
    uint32_t           temp = uart_->CR2;
    RegisterUtils::clearBits(temp, Mask);
    switch (config_.stopbits) {
        case UartStopBit::STOP_1:
            break;
        case UartStopBit::STOP_2:
            RegisterUtils::setBits(temp, 0x10 << 12);
            break;
        default:
            SetError(UartError::InvalidConfig);
            return;
    }
    uart_->CR2 = temp;
}

void Stm32Uart::enablePeripheralClock() {
    volatile uint32_t* enrReg    = nullptr;
    uint32_t           enableBit = 0;

    switch (config_.dev_num) {
        case UartNum::USART_D1: {
            enrReg    = &RCC->APB2ENR;
            enableBit = RCC_APB2ENR_USART1_EN;
            break;
        }
        case UartNum::USART_D2: {
            enrReg    = &RCC->APB1ENR;
            enableBit = RCC_APB1ENR_USART2_EN;
            break;
        }
        case UartNum::USART_D6: {
            enrReg    = &RCC->APB2ENR;
            enableBit = RCC_APB2ENR_USART6_EN;
            break;
        }
        default:
            SetError(UartError::InvalidConfig);
            return;
    }

    RegisterUtils::setBits(*enrReg, enableBit);

    // Ensure RCC write completion before peripheral access
    (void)(*enrReg);
}

void Stm32Uart::disablePeripheralClock() {
    volatile uint32_t* clrReg   = nullptr;
    uint32_t           clearBit = 0;

    switch (config_.dev_num) {
        case UartNum::USART_D1: {
            clrReg   = &RCC->APB2ENR;
            clearBit = RCC_APB2ENR_USART1_EN;
            break;
        }
        case UartNum::USART_D2: {
            clrReg   = &RCC->APB1ENR;
            clearBit = RCC_APB1ENR_USART2_EN;
            break;
        }
        case UartNum::USART_D6: {
            clrReg   = &RCC->APB2ENR;
            clearBit = RCC_APB2ENR_USART6_EN;
            break;
        }
        default:
            SetError(UartError::InvalidConfig);
            return;
    }
    RegisterUtils::clearBits(*clrReg, clearBit);
}
void Stm32Uart::configureComm() {
    constexpr uint32_t mask = USART_CR1_TE | USART_CR1_RE;
    uint32_t           temp = uart_->CR1;
    RegisterUtils::clearBits(temp, mask);

    switch (config_.comm) {
        case UartComm::RX_ONLY:
            RegisterUtils::setBits(temp, USART_CR1_RE);
            rx_state_ = UartState::Ready;
            break;
        case UartComm::TX_ONLY:
            RegisterUtils::setBits(temp, USART_CR1_TE);
            tx_state_ = UartState::Ready;
            break;
        case UartComm::RX_TX:
            RegisterUtils::setBits(temp, mask);
            tx_state_ = UartState::Ready;
            rx_state_ = UartState::Ready;
            break;
    }
    uart_->CR1 = temp;
}
void Stm32Uart::enablePeripheral() {
    RegisterUtils::setBits(uart_->CR1, USART_CR1_UE);
}

void Stm32Uart::disablePeripheral() {
    RegisterUtils::clearBits(uart_->CR1, USART_CR1_UE);
}

bool Stm32Uart::send(const uint8_t* data, size_t DataLength) {
    if (tx_state_ != UartState::Ready) {
        return false;
    }
    tx_state_ = UartState::BusyTx;

    for (size_t i = 0; i < DataLength; ++i) {
        while (!(uart_->SR & USART_SR_TXE));
        uart_->DR = data[i];
    }
    while (!(uart_->SR & USART_SR_TC));

    tx_state_ = UartState::Ready;

    return true;
}
bool Stm32Uart::receive(uint8_t* buffer, size_t DataLength) {
    if (rx_state_ != UartState::Ready) {
        return false;
    }

    rx_state_ = UartState::BusyRx;

    for (size_t i = 0; i < DataLength; ++i) {
        while (!(uart_->SR & USART_SR_RXNE));
        buffer[i] = uart_->DR;
    }

    rx_state_ = UartState::Ready;
    return true;
}

void Stm32Uart::setVariable(USART_TypeDef* uart, const UartConfig& config) {
    uart_   = uart;
    config_ = config;
}
void Stm32Uart::clearFlag() {
    volatile uint32_t temp;
    temp = uart_->SR;
    temp = uart_->DR;
    (void)temp;
}