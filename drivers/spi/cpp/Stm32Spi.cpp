#include "Stm32Spi.hpp"

#include "RegisterUtils.hpp"
#include "low-level/spi_registers.h"
#include "pch.hpp"

static const peripherals_regs_table<SPI_TypeDef> spi_rcc_table[static_cast<uint8_t>(Stm32Spi::SpiDev::COUNT)]{
    {SPI1, RCC_APB2ENR_SPI1_EN, RCC_APB2RSTR_SPI1_RST},
    {SPI2, RCC_APB1ENR_SPI2_EN, RCC_APB1RSTR_SPI2_RST},
    {SPI3, RCC_APB1ENR_SPI3_EN, RCC_APB1RSTR_SPI3_RST},
    {SPI4, RCC_APB2ENR_SPI4_EN, RCC_APB2RSTR_SPI4_RST}
};

Result<> Stm32Spi::initialize(const Config& cfg) {
    /* assign instance */
    spi_ = spi_rcc_table[static_cast<uint8_t>(cfg.dev)].instance;

    if (spi_ == nullptr) return Fail(Err::NullInstance);

    /* Enable Clock */
    if (spi_ == SPI1 || spi_ == SPI4) {
        RegisterUtils::setBits(RCC->APB2ENR, spi_rcc_table[static_cast<uint8_t>(cfg.dev)].enableBit);
    } else {
        RegisterUtils::setBits(RCC->APB1ENR, spi_rcc_table[static_cast<uint8_t>(cfg.dev)].enableBit);
    }

    uint32_t mask = spi_->CR1;
    /* Master / Slave */
    if (cfg.mode == Mode::Master) RegisterUtils::setBits(mask, SPI_CR1_MSTR);

    /* CPOL / CPHA */
    if (cfg.cpol == ClockPolarity::IdleHigh) RegisterUtils::setBits(mask, SPI_CR1_CPOL);
    if (cfg.cpha == ClockPhase::SecondEdge) RegisterUtils::setBits(mask, SPI_CR1_CPHA);

    /* Baud Rate Prescalar */
    RegisterUtils::setBits(mask, cfg.baudRatePrescalar << SPI_CR1_BRR_Pos);

    /* Data Frame Size */
    if (cfg.dataSize == DataSize::Bits16) RegisterUtils::setBits(mask, SPI_CR1_DFF);

    /* Bit Order */
    if (cfg.bitOrder == BitOrder::LsbFirst) RegisterUtils::setBits(mask, SPI_CR1_LSB);

    if (cfg.nssSoftware) {
        RegisterUtils::setBits(mask, SPI_CR1_SSM);
        RegisterUtils::setBits(mask, SPI_CR1_SSI);
    }

    /* Set Transmission Only*/
    RegisterUtils::setBits(mask, SPI_CR1_BIDIMODE);
    RegisterUtils::setBits(mask, SPI_CR1_BIDIOE);

    spi_->CR1 = mask;

    /* Enable SPI */
    RegisterUtils::setBits(spi_->CR1, SPI_CR1_SPE);

    return Ok();
}
void Stm32Spi::transferOnly(const uint8_t* txBuf, size_t len) {
    if (len == 0) return;

    if (state_ != State::Idle) {
        return;
    }

    state_ = State::Transmitting;

    for (size_t i = 0; i < len; ++i) {
        while (!(spi_->SR & SPI_SR_TXE));
        spi_->DR = txBuf[i];
    }
    while (spi_->SR & SPI_SR_BSY);
    state_ = State::Idle;
}
bool Stm32Spi::isBusy() const {
    const uint32_t sr = spi_->SR;
    return sr & SPI_SR_BSY;
}
SPI_TypeDef* Stm32Spi::rawInstance() const {
    return spi_;
}
