#include "Stm32Uart.hpp"

#include <cstdint>

#include "RegisterUtils.hpp"
#include "Result.hpp"
#include "drivers.hpp"
#include "low-level/nvic.h"
#include "low-level/rcc.h"
#include "low-level/rcc_bitfields.h"
#include "low-level/uart_bitfields.h"
#include "low-level/uart_registers.h"
#include "low-level/uart_types.h"

namespace {
static const peripherals_regs_table<USART_TypeDef> uart_table[static_cast<uint8_t>(UartDevice_t::USART_COUNT)] = {
    {USART1, RCC_APB2ENR_USART1_EN, RCC_APB2RSTR_USART1_RST},
    {USART2, RCC_APB1ENR_USART2_EN, RCC_APB1RSTR_USART2_RST},
    {USART6, RCC_APB2ENR_USART6_EN, RCC_APB2RSTR_USART6_RST}
};

uint32_t BaudRateCalc(uint32_t baud, uint32_t fck, uint32_t over) {
    float_t nom_usart = (float_t)fck / (float_t)(8U * (2 - over) * baud);
    float_t div_usart = ceilf((nom_usart - (uint32_t)nom_usart) * 16);
    return (uint32_t)nom_usart << 4 | (uint32_t)div_usart << 0;
}

USART_TypeDef* enableAndGet(UartDevice_t dev) {
    const auto& entry = uart_table[static_cast<uint8_t>(dev)];
    RegisterUtils::setBits(entry.instance == USART2 ? RCC->APB1ENR : RCC->APB2ENR, entry.enableBit);
    (void)RCC->APB1ENR;
    (void)RCC->APB2ENR;
    return entry.instance;
}

}  // namespace

Stm32Uart::Stm32Uart() : m_Init(false) {}

Result<> Stm32Uart::initialize(const UartConfig& cfg) {
    uart_ = enableAndGet(cfg.dev_num);

    if (uart_ == nullptr) return Fail(Err::NullInstance);

    /* Disable USART */
    RegisterUtils::clearBits(uart_->CR1, USART_CR1_UE);
    setComm(cfg.comm);
    setBaudRate(cfg.baudRate);
    setParity(cfg.parity);
    setStopBits(cfg.stopbits);

    /* Enable USART */
    RegisterUtils::setBits(uart_->CR1, USART_CR1_UE);
    m_Init = true;
    return Ok();
}

bool Stm32Uart::send(const uint8_t* data, size_t DataLength) {
    if (tx_state_ != UartState_t::Ready) {
        return false;
    }

    tx_state_ = UartState_t::BusyTx;

    for (size_t i = 0; i < DataLength; ++i) {
        uint32_t start = getDrivers().my_systick.get_ticks();
        while (!(uart_->SR & USART_SR_TXE)) {
            if (getDrivers().my_systick.get_ticks() - start > kTimeout) {
                tx_state_ = UartState_t::Error;
                error_    = UartError_t::Timeout;
                return false;
            }
        }
        uart_->DR = data[i];
    }

    uint32_t start = getDrivers().my_systick.get_ticks();
    while (!(uart_->SR & USART_SR_TC)) {
        if (getDrivers().my_systick.get_ticks() - start > kTimeout) {
            tx_state_ = UartState_t::Error;
            error_    = UartError_t::Timeout;
            return false;
        }
    }

    tx_state_ = UartState_t::Ready;

    return true;
}

bool Stm32Uart::receive(uint8_t* buffer, size_t DataLength) {
    if (rx_state_ != UartState_t::Ready) {
        return false;
    }

    rx_state_ = UartState_t::BusyRx;

    for (size_t i = 0; i < DataLength; ++i) {
        uint32_t start = getDrivers().my_systick.get_ticks();
        while (!(uart_->SR & USART_SR_RXNE)) {
            if (getDrivers().my_systick.get_ticks() - start > kTimeout) {
                rx_state_ = UartState_t::Error;
                error_    = UartError_t::Timeout;
                return false;
            }
        }
        buffer[i] = uart_->DR;
    }

    rx_state_ = UartState_t::Ready;
    return true;
}

void Stm32Uart::setBaudRate(UartBaudRate_t baudrate) {
    uint32_t baud = static_cast<uint32_t>(baudrate);
    uint32_t fck  = (uart_ == USART2) ? getDrivers().sysclock.getSysClock().apb1 : getDrivers().sysclock.getSysClock().apb2;
    uart_->BRR    = BaudRateCalc(baud, fck, 0);
}

void Stm32Uart::setParity(UartParity_t parity) {
    /* Clear Parity Bit Mask First */
    RegisterUtils::clearBits(uart_->CR1, USART_CR1_PCE | USART_CR1_PS);

    switch (parity) {
        case UartParity_t::NONE:
            break;
        case UartParity_t::EVEN:
            RegisterUtils::setBits(uart_->CR1, USART_CR1_PCE);
            break;
        case UartParity_t::ODD:
            RegisterUtils::setBits(uart_->CR1, USART_CR1_PCE | USART_CR1_PS);
            break;
    }
}

void Stm32Uart::setStopBits(UartStopBit_t stopbit) {
    switch (stopbit) {
        case UartStopBit_t::USART_CR2_STOP_1:
            break;
        case UartStopBit_t::USART_CR2_STOP_2:
            RegisterUtils::setBits(uart_->CR2, static_cast<uint8_t>(UartStopBit_t::USART_CR2_STOP_2) << 12);
            break;
    }
}

void Stm32Uart::setComm(UartComm_t comm) {
    RegisterUtils::clearBits(uart_->CR1, USART_CR1_TE | USART_CR1_RE);

    switch (comm) {
        case UartComm_t::RX_ONLY:
            RegisterUtils::setBits(uart_->CR1, USART_CR1_RE);
            rx_state_ = UartState_t::Ready;
            break;
        case UartComm_t::TX_ONLY:
            RegisterUtils::setBits(uart_->CR1, USART_CR1_TE);
            tx_state_ = UartState_t::Ready;
            break;
        case UartComm_t::RX_TX:
            RegisterUtils::setBits(uart_->CR1, USART_CR1_TE | USART_CR1_RE);
            tx_state_ = UartState_t::Ready;
            rx_state_ = UartState_t::Ready;
            break;
    }
}

void Stm32Uart::clearFlag() {
    volatile uint32_t temp;
    temp = uart_->SR;
    temp = uart_->DR;
    (void)temp;
}

USART_TypeDef* Stm32Uart::rawInstance() const {
    return uart_;
}

Result<> Stm32Uart::enable_nvic(USART_TypeDef* dev) {
    if (dev == USART1) {
        My_NVIC_EnableIRQ(USART1_IRQn);
    } else if (dev == USART2) {
        My_NVIC_EnableIRQ(USART2_IRQn);
    } else if (dev == USART6) {
        My_NVIC_EnableIRQ(USART6_IRQn);
    } else {
        return Fail(Err::NullInstance);
    }
    return Ok();
}
