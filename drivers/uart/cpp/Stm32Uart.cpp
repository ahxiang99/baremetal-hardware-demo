#include "Stm32Uart.hpp"

#include <cstdint>

#include "IUart.hpp"
#include "RegisterUtils.hpp"
#include "drivers.hpp"
#include "low-level/nvic.h"
#include "low-level/rcc.h"
#include "low-level/rcc_bitfields.h"
#include "low-level/uart_registers.h"
#include "low-level/uart_types.h"

constexpr uint32_t Timeout = 3U;

struct uart_regs_table {
    USART_TypeDef* instance;
    uint32_t       enableBit;
    uint32_t       disableBit;
};

static const uart_regs_table uart_table[static_cast<uint8_t>(UartNum::Dev_Total)] = {
    {USART1, RCC_APB2ENR_USART1_EN, RCC_APB2RSTR_USART1_RST},
    {USART2, RCC_APB1ENR_USART2_EN, RCC_APB1RSTR_USART2_RST},
    {USART6, RCC_APB2ENR_USART6_EN, RCC_APB2RSTR_USART6_RST}
};

Stm32Uart::Stm32Uart(const UartConfig& config) : config_(config) {
    uart_ = uart_table[static_cast<uint8_t>(config_.dev_num)].instance;
}

void Stm32Uart::configure(const UartConfig& config) {
    config_ = config;
    uart_   = uart_table[static_cast<uint8_t>(config_.dev_num)].instance;
}

bool Stm32Uart::initialize() {
    enablePeripheralClock();
    disablePeripheral();

    configureComm();
    configureBaudRate();
    configureParity();
    configureStopBits();

    enablePeripheral();
    onPostUartInit();
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
            error_ = UartError::InvalidConfig;
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
            error_ = UartError::InvalidConfig;
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
            My_NVIC_EnableIRQ(USART1_IRQn);
            break;
        }
        case UartNum::USART_D2: {
            enrReg    = &RCC->APB1ENR;
            enableBit = RCC_APB1ENR_USART2_EN;
            My_NVIC_EnableIRQ(USART2_IRQn);
            break;
        }
        case UartNum::USART_D6: {
            enrReg    = &RCC->APB2ENR;
            enableBit = RCC_APB2ENR_USART6_EN;
            My_NVIC_EnableIRQ(USART6_IRQn);
            break;
        }
        default:
            error_ = UartError::InvalidConfig;
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
            error_ = UartError::InvalidConfig;
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
        default:
            error_ = UartError::InvalidConfig;
            return;
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
    tx_state_              = UartState::BusyTx;

    uint32_t measure_start = 0;
    for (size_t i = 0; i < DataLength; ++i) {
        measure_start = getDriver().my_systick.get_ticks();
        while (!(uart_->SR & USART_SR_TXE)) {
            if ((getDriver().my_systick.get_ticks() - measure_start) > Timeout) {
                error_    = UartError::Timeout;
                tx_state_ = UartState::Error;
                return false;
            }
        }
        uart_->DR = data[i];
    }
    measure_start = getDriver().my_systick.get_ticks();
    while (!(uart_->SR & USART_SR_TC)) {
        if ((getDriver().my_systick.get_ticks() - measure_start) > Timeout) {
            error_    = UartError::Timeout;
            tx_state_ = UartState::Error;
            return false;
        }
    }
    tx_state_ = UartState::Ready;
    return true;
}
bool Stm32Uart::receive(uint8_t* buffer, size_t DataLength) {
    if (rx_state_ != UartState::Ready) {
        return false;
    }

    rx_state_              = UartState::BusyRx;

    uint32_t measure_start = 0;
    for (size_t i = 0; i < DataLength; ++i) {
        measure_start = getDriver().my_systick.get_ticks();
        while (!(uart_->SR & USART_SR_RXNE)) {
            if ((getDriver().my_systick.get_ticks() - measure_start) > Timeout) {
                error_    = UartError::Timeout;
                tx_state_ = UartState::Error;
                return false;
            }
        }
        buffer[i] = uart_->DR;
    }

    rx_state_ = UartState::Ready;
    return true;
}

void Stm32Uart::clearFlag() {
    volatile uint32_t temp;
    temp = uart_->SR;
    temp = uart_->DR;
    (void)temp;
}
