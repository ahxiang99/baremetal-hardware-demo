#include "Stm32Uart.hpp"

#include <cstdint>

#include "RegisterUtils.hpp"
#include "drivers.hpp"
#include "low-level/nvic.h"
#include "low-level/rcc.h"
#include "low-level/uart_types.h"

static const peripherals_regs_table<USART_TypeDef> uart_table[static_cast<uint8_t>(UartNum::Dev_Total)] = {
    {USART1, RCC_APB2ENR_USART1_EN, RCC_APB2RSTR_USART1_RST},
    {USART2, RCC_APB1ENR_USART2_EN, RCC_APB1RSTR_USART2_RST},
    {USART6, RCC_APB2ENR_USART6_EN, RCC_APB2RSTR_USART6_RST}
};

uint32_t BaudRateCalc(uint32_t baud, uint32_t fck, uint32_t over) {
    float_t nom_usart = (float_t)fck / (float_t)(8U * (2 - over) * baud);
    float_t div_usart = ceilf((nom_usart - (uint32_t)nom_usart) * 16);
    return (uint32_t)nom_usart << 4 | (uint32_t)div_usart << 0;
}

Stm32Uart::Stm32Uart() {}

Stm32Uart::Stm32Uart(const UartConfig& config) : config_(config) {
    uart_ = uart_table[static_cast<uint8_t>(config_.dev_num)].instance;
}

bool Stm32Uart::initialize() {
    enablePeripheralClock();
    disablePeripheral();
    configureComm();
    configureBaudRate();
    configureParity();
    configureStopBits();
    enablePeripheral();
    m_Init = true;
    return true;
}

void Stm32Uart::configure(const UartConfig& config) {
    config_ = config;
    uart_   = uart_table[static_cast<uint8_t>(config_.dev_num)].instance;
}

bool Stm32Uart::send(const uint8_t* data, size_t DataLength) {
    if (tx_state_ != UartState::Ready) {
        return false;
    }

    tx_state_ = UartState::BusyTx;

    for (size_t i = 0; i < DataLength; ++i) {
        uint32_t start = getDrivers().my_systick.get_ticks();
        while (!(uart_->SR & USART_SR_TXE)) {
            if (getDrivers().my_systick.get_ticks() - start > Timeout) {
                tx_state_ = UartState::Error;
                error_    = UartError::Timeout;
                return false;
            }
        }
        uart_->DR = data[i];
    }

    uint32_t start = getDrivers().my_systick.get_ticks();
    while (!(uart_->SR & USART_SR_TC)) {
        if (getDrivers().my_systick.get_ticks() - start > Timeout) {
            tx_state_ = UartState::Error;
            error_    = UartError::Timeout;
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

    rx_state_ = UartState::BusyRx;

    for (size_t i = 0; i < DataLength; ++i) {
        uint32_t start = getDrivers().my_systick.get_ticks();
        while (!(uart_->SR & USART_SR_RXNE)) {
            if (getDrivers().my_systick.get_ticks() - start > Timeout) {
                rx_state_ = UartState::Error;
                error_    = UartError::Timeout;
                return false;
            }
        }
        buffer[i] = uart_->DR;
    }

    rx_state_ = UartState::Ready;
    return true;
}

void Stm32Uart::configureBaudRate() {
    uint32_t baudReg = 0;
    switch (config_.dev_num) {
        case UartNum::USART_D1:
        case UartNum::USART_D6:
            baudReg = BaudRateCalc(static_cast<uint32_t>(config_.baudRate), getDrivers().sysclock.getSysClock().apb2, 0);
            break;

        case UartNum::USART_D2:
            baudReg = BaudRateCalc(static_cast<uint32_t>(config_.baudRate), getDrivers().sysclock.getSysClock().apb1, 0);
            break;
        default:
            error_ = UartError::InvalidConfig;
            return;
    }
    uart_->BRR = baudReg;
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
        case UartStopBit::USART_CR2_STOP_1:
            break;
        case UartStopBit::USART_CR2_STOP_2:
            RegisterUtils::setBits(temp, static_cast<uint8_t>(UartStopBit::USART_CR2_STOP_2) << 12);
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
    }
    uart_->CR1 = temp;
}
void Stm32Uart::enablePeripheral() {
    RegisterUtils::setBits(uart_->CR1, USART_CR1_UE);
}

void Stm32Uart::disablePeripheral() {
    RegisterUtils::clearBits(uart_->CR1, USART_CR1_UE);
}

void Stm32Uart::clearFlag() {
    volatile uint32_t temp;
    temp = uart_->SR;
    temp = uart_->DR;
    (void)temp;
}

void Stm32Uart::enableNVICInterrupt() {
    switch (config_.dev_num) {
        case UartNum::USART_D1:
            My_NVIC_EnableIRQ(USART1_IRQn);
            break;
        case UartNum::USART_D2:
            My_NVIC_EnableIRQ(USART2_IRQn);
            break;

        case UartNum::USART_D6:
            My_NVIC_EnableIRQ(USART6_IRQn);
            break;
        case UartNum::Dev_Total:
            error_ = UartError::InvalidConfig;
            break;
    }
}
