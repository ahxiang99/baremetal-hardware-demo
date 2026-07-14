#pragma once

#include "low-level/spi_bitfields.h"
#include "low-level/spi_registers.h"

class Stm32Spi {
   public:
    enum class SpiDev { SPI_D1, SPI_D2, SPI_D3, SPI_D4, COUNT };
    enum class Mode { Master, Slave };
    enum class ClockPolarity { IdleLow, IdleHigh };
    enum class ClockPhase { FirstEdge, SecondEdge };
    enum class DataSize { Bits8, Bits16 };
    enum class BitOrder { MsbFirst, LsbFirst };

    struct Config {
        SpiDev        dev;
        Mode          mode;
        ClockPolarity cpol;
        ClockPhase    cpha;
        DataSize      dataSize;
        BitOrder      bitOrder;
        uint32_t      baudRatePrescalar;
        bool          nssSoftware;
    };

    Result<>     initialize(const Config& cfg);
    void         transferOnly(const uint8_t* txBuf, size_t len);
    bool         isBusy() const;

    SPI_TypeDef* rawInstance() const;

   private:
    enum class State { Idle, Transmitting, Receiving, Error };
    State        state_ = State::Idle;
    SPI_TypeDef* spi_;
};