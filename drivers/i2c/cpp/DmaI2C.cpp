#include "DmaI2C.hpp"

#include <cstddef>
#include <cstdint>

#include "RegisterUtils.hpp"
#include "Stm32I2C.hpp"
#include "cpp/Dma.hpp"
#include "drivers.hpp"
#include "logger.hpp"
#include "low-level/DMA_bitfields.h"
#include "low-level/i2c_bitfields.h"
#include "low-level/nvic.h"
#include "low-level/rcc_bitfields.h"
#include "pch.hpp"

bool DmaI2C::initialize() {
    if (!Stm32I2C::initialize()) {
        return false;
    }
    return true;
}

void DmaI2C::onPostI2CInit() {
    initTxDma();
    initRxDma();
    My_NVIC_EnableIRQ(DMA1_Stream0_IRQn);
    My_NVIC_EnableIRQ(DMA1_Stream7_IRQn);
}

void DmaI2C::initTxDma() {
    DMA_Config config;
    config.DMA_BaseAddress = DMA1;
    config.Channel         = DMA_Channel::Channel_1;
    config.Stream          = DMA_Stream::Stream_7;
    config.Direction       = DMA_Direction::DMA_MEMORY_TO_PERIPH;
    config.Mode            = DMA_Mode::Normal;

    hdmatx.setConfig(config);
    hdmatx.initialize();

    /* Setup Transfer Completion Callback */
    hdmatx.setXferCpltCallback(
        [](void* ctx) {
            auto* self = static_cast<DmaI2C*>(ctx);
            self->push_isr_event(I2C_IsrTag::TX_DMA_DONE);
            self->dma_complete_ = true;
        },
        this);
}
void DmaI2C::initRxDma() {
    DMA_Config config;
    config.DMA_BaseAddress = DMA1;
    config.Channel         = DMA_Channel::Channel_1;
    config.Stream          = DMA_Stream::Stream_0;
    config.Direction       = DMA_Direction::DMA_PERIPH_TO_MEMORY;
    config.Mode            = DMA_Mode::Normal;

    hdmarx.setConfig(config);
    hdmarx.initialize();
    RegisterUtils::clearBits(hdmarx.getDmaStream()->CR, DMA_SCR_CIRC);
    /* Setup Transfer Completion Callback */
    hdmarx.setXferCpltCallback(
        [](void* ctx) {
            auto* self = static_cast<DmaI2C*>(ctx);
            self->push_isr_event(I2C_IsrTag::RX_DMA_DONE);
            self->RxEventFlag   = true;
            self->dma_complete_ = true;
        },
        this);
}

bool DmaI2C::Write(uint16_t DevAddress, const uint8_t* pData, uint16_t Size, uint32_t Timeout) {
    if (state_ != I2C_State::READY) {
        return false;
    }
    if (isHardwareBusy(Timeout)) {
        error_ = I2C_Error::ERR_I2C_BUS;
        Error_Handler();
        return false;
    }

    if (Size == 0 || pData == nullptr) {
        error_ = I2C_Error::ERR_I2C_DATA_EMPTY;
        return false;
    }

    std::copy(pData, pData + Size, dmaTxBuffer.data());
    DevAddr       = DevAddress;
    XferSize      = Size;
    state_        = I2C_State::BUSY_TX;
    mode_         = I2C_Mode::MASTER;
    error_        = I2C_Error::NONE;
    dma_complete_ = false;

    if (Size == 1) {
        dma_complete_ = true;
        disable_DMA_request();
        enableInterrupt();
        generateStartCondition();
    } else {
        enable_DMA_request_tx();
        hdmatx.StartDataStream(reinterpret_cast<uint32_t>(dmaTxBuffer.data()), (uint32_t)&i2c_->DR,
                               Size);
        /* Get Acknowledgement from Slave */
        enableInterrupt();
        generateStartCondition();
    }

    return true;
}
bool DmaI2C::Read(uint16_t DevAddress, uint8_t* pData, uint16_t Size, uint32_t Timeout) {
    LOG_DEBUG("Read() called: state={} SR2={}", static_cast<uint32_t>(state_), (Hex)i2c_->SR2);
    if (state_ != I2C_State::READY) {
        return false;
    }

    if (isHardwareBusy(Timeout)) {
        error_ = I2C_Error::ERR_I2C_BUS;
        return false;
    }

    if (Size == 0 || pData == nullptr) {
        error_ = I2C_Error::ERR_I2C_DATA_EMPTY;
        return false;
    }

    XferPtr       = pData;
    XferSize      = Size;
    DevAddr       = static_cast<uint8_t>(DevAddress);
    state_        = I2C_State::BUSY_RX;
    mode_         = I2C_Mode::MASTER;
    error_        = I2C_Error::NONE;
    dma_complete_ = false;
    RxEventFlag   = false;
    rxDmaPending_ = true;

    enable_DMA_request_rx();
    enableAckBit();
    enableInterrupt();
    generateStartCondition();
    return true;
}

void DmaI2C::handleEVInterrupt() {
    const uint32_t  sr1  = i2c_->SR1;
    const uint32_t  sr2  = i2c_->SR2;  // reading SR2 also helps clear ADDR in some cases
    const I2C_State snap = state_;

    push_isr_event(I2C_IsrTag::EV_ENTRY, sr1, sr2);

    // ── SB: Start condition generated ────────────────────────────────────────
    if (sr1 & I2C_SR1_SB) {
        push_isr_event(I2C_IsrTag::EV_SB, static_cast<uint32_t>(snap), DevAddr);
        if (snap == I2C_State::BUSY_TX) {
            i2c_->DR = DevAddr & ~(1u << 0);  // write address
        } else if (snap == I2C_State::BUSY_RX) {
            i2c_->DR = DevAddr | (1u << 0);  // read address
        } else {
            generateStopCondition();
            error_ = I2C_Error::ERR_I2C_SB;
            state_ = I2C_State::READY;
        }
        return;
    }

    // ── ADDR: Address sent and ACKed by slave ─────────────────────────────────
    // During TX: slave ACKed address, DMA is already running — just clear flag.
    // During RX: arm DMA first, THEN clear (releases clock stretch).
    if (sr1 & I2C_SR1_ADDR) {
        if (snap == I2C_State::BUSY_RX && rxDmaPending_) {
            push_isr_event(I2C_IsrTag::EV_ADDR_RX, XferSize);
            rxDmaPending_ = false;
            hdmarx.StartDataStream(reinterpret_cast<uint32_t>(&i2c_->DR),
                                   reinterpret_cast<uint32_t>(rxBuffer.data()), XferSize);
        } else {
            push_isr_event(I2C_IsrTag::EV_ADDR_TX);
            // TX path — DMA is already streaming, nothing to do here
        }
        clearAddrFlag();  // must happen after DMA armed in RX path
        return;
    }

    // ── BTF: Byte Transfer Finished ───────────────────────────────────────────
    // This fires after DMA completes AND the last byte has been shifted out.
    // Only issue STOP here for TX, and only when DMA is actually done.
    if (sr1 & I2C_SR1_BTF) {
        if (snap == I2C_State::BUSY_TX) {
            if (dma_complete_) {
                push_isr_event(I2C_IsrTag::EV_BTF_STOP);
                generateStopCondition();
                disableInterrupt();
                state_        = I2C_State::READY;
                mode_         = I2C_Mode::NONE;
                dma_complete_ = false;
            } else {
                // DMA TC hasn't fired yet — stall BTF by reading DR
                // This clears BTF without advancing the state machine
                push_isr_event(I2C_IsrTag::EV_BTF_STALL);
                volatile uint32_t dummy = i2c_->DR;
                (void)dummy;
            }
        }
        return;
    }

    // ── TXE alone: DMA not done yet, do NOT generate STOP ────────────────────
    if (sr1 & I2C_SR1_TXE) {
        if (snap == I2C_State::BUSY_TX && dma_complete_) {
            // Single-byte path: write the byte directly on TXE
            // Only if DMA is not running (dma_complete_ pre-set to true)
            if (!(i2c_->CR2 & I2C_CR2_DMAEN)) {
                push_isr_event(I2C_IsrTag::EV_TXE, dmaTxBuffer[0]);
                i2c_->DR = dmaTxBuffer[0];
                RegisterUtils::clearBits(i2c_->CR2, I2C_CR2_ITBUFEN);
            }
        }
        return;
    }
}
void DmaI2C::handleERInterrupt() {
    const uint32_t sr1 = i2c_->SR1;

    // 1. Acknowledge Failure (AF) - The Sensor didn't respond
    if (sr1 & I2C_SR1_AF) {
        push_isr_event(I2C_IsrTag::ER_AF, sr1);
    }

    // 2. Bus Error (BERR) - Misplaced Start/Stop condition
    if (sr1 & I2C_SR1_BERR) {
        push_isr_event(I2C_IsrTag::ER_BERR, sr1);
    }

    // 3. Arbitration Lost (ARLO) - Another Master took the bus
    if (sr1 & I2C_SR1_ARLO) {
        push_isr_event(I2C_IsrTag::ER_ARLO, sr1);
    }

    // 4. Overrun/Underrun (OVR) - CPU too slow for the clock speed
    if (sr1 & I2C_SR1_OVR) {
        push_isr_event(I2C_IsrTag::ER_OVR, sr1);
    }

    // CRITICAL: Disable interrupts so we don't loop forever in an error state
    RegisterUtils::clearBits(i2c_->CR2, I2C_CR2_ITEVTEN | I2C_CR2_ITBUFEN | I2C_CR2_ITERREN);
}

bool DmaI2C::isHardwareBusy(const uint32_t& Timeout) {
    volatile uint32_t count = Timeout * 15999;
    do {
        count = count - 1;
        if (count == 0U) {
            return true;
        }
    } while (i2c_->SR2 & I2C_SR2_BUSY);
    return false;
}
void DmaI2C::handleTxDmaInterrupt() {
    push_isr_event(I2C_IsrTag::TX_DMA_FIRED);
    hdmatx.handleInterrupt();
}
void DmaI2C::handleRxDmaInterrupt() {
    hdmarx.handleInterrupt();
}
void DmaI2C::enable_DMA_request_tx() {
    /* Enable I2C DMA Request */
    RegisterUtils::setBits(i2c_->CR2, I2C_CR2_DMAEN);
    RegisterUtils::clearBits(i2c_->CR2, I2C_CR2_LAST);
}
void DmaI2C::enable_DMA_request_rx() {
    /* Enable I2C DMA Request */
    RegisterUtils::setBits(i2c_->CR2, I2C_CR2_DMAEN | I2C_CR2_LAST);
}

void DmaI2C::enableInterrupt() {
    constexpr uint32_t mask = I2C_CR2_ITEVTEN | I2C_CR2_ITBUFEN | I2C_CR2_ITERREN;
    RegisterUtils::setBits(i2c_->CR2, mask);
}
void DmaI2C::disableInterrupt() {
    constexpr uint32_t mask = I2C_CR2_ITEVTEN | I2C_CR2_ITBUFEN | I2C_CR2_ITERREN;
    RegisterUtils::clearBits(i2c_->CR2, mask);
}
void DmaI2C::onDataReceived(RxCallbackFn fn, void* ctx1, void* ctx2) {
    rx_fn_   = fn;
    rx_ctx1_ = ctx1;
    rx_ctx2_ = ctx2;
}
void DmaI2C::processRx() {
    drain_isr_log();
    if (!RxEventFlag) return;
    RxEventFlag = false;

    if (dma_complete_) {
        /* Rx Finally Completed */
        generateStopCondition();
        state_ = I2C_State::READY;
        mode_  = I2C_Mode::NONE;
    }

    /* Update Rx Buffer Head */
    for (size_t i = 0; i < XferSize; ++i) {
        XferPtr[i] = rxBuffer[i];
    }
    if (rx_fn_) rx_fn_(rx_ctx1_, rx_ctx2_);
}
void DmaI2C::disable_DMA_request() {
    RegisterUtils::clearBits(i2c_->CR2, I2C_CR2_DMAEN | I2C_CR2_LAST);
}

#ifndef NDEBUG
void DmaI2C::push_isr_event(I2C_IsrTag tag, uint32_t a, uint32_t b) {
    uint8_t h                 = isr_log_head_;
    isr_log_[h % kIsrLogSize] = {tag, a, b};
    isr_log_head_             = static_cast<uint8_t>(h + 1u);
}

void DmaI2C::drain_isr_log() {
    while (isr_log_tail_ != static_cast<uint8_t>(isr_log_head_)) {
        const auto& e = isr_log_[isr_log_tail_ % kIsrLogSize];
        switch (e.tag) {
            case I2C_IsrTag::EV_ENTRY:
                LOG_DEBUG("EV ISR: SR1={} SR2={}", (Hex)e.a, (Hex)e.b);
                break;
            case I2C_IsrTag::EV_SB:
                LOG_DEBUG("SB: state={} DevAddr={}", e.a, (Hex)e.b);
                break;
            case I2C_IsrTag::EV_ADDR_RX:
                LOG_DEBUG("ADDR RX: arming DMA, XferSize={}", e.a);
                break;
            case I2C_IsrTag::EV_ADDR_TX:
                LOG_DEBUG("ADDR TX: slave ACKed");
                break;
            case I2C_IsrTag::EV_BTF_STOP:
                LOG_DEBUG("BTF+DMA done: generating STOP (TX)");
                break;
            case I2C_IsrTag::EV_BTF_STALL:
                LOG_DEBUG("BTF before DMA done: clearing BTF via DR read");
                break;
            case I2C_IsrTag::EV_TXE:
                LOG_DEBUG("TXE single-byte: writing {}", (Hex)e.a);
                break;
            case I2C_IsrTag::ER_AF:
                LOG_DEBUG("ER: AF - Slave NACK, SR1={}", (Hex)e.a);
                break;
            case I2C_IsrTag::ER_BERR:
                LOG_DEBUG("ER: BERR - bus error, SR1={}", (Hex)e.a);
                break;
            case I2C_IsrTag::ER_ARLO:
                LOG_DEBUG("ER: ARLO - arbitration lost, SR1={}", (Hex)e.a);
                break;
            case I2C_IsrTag::ER_OVR:
                LOG_DEBUG("ER: OVR, SR1={}", (Hex)e.a);
                break;
            case I2C_IsrTag::TX_DMA_FIRED:
                LOG_DEBUG("TX DMA ISR fired");
                break;
            case I2C_IsrTag::TX_DMA_DONE:
                LOG_DEBUG("TX DMA TC callback fired");
                break;
            case I2C_IsrTag::RX_DMA_DONE:
                LOG_DEBUG("RX DMA TC callback fired");
                break;
        }
        isr_log_tail_ = static_cast<uint8_t>(isr_log_tail_ + 1u);
    }
}
#else
void DmaI2C::push_isr_event(I2C_IsrTag, uint32_t, uint32_t) {}
void DmaI2C::drain_isr_log() {}
#endif
void DmaI2C::Error_Handler() {
    if (error_ == I2C_Error::ERR_I2C_BUS) {
        /* Bus Stuck */
        RegisterUtils::setBits(i2c_->CR1, I2C_CR1_SWRST);
        /* Check again if bus busy is clear */
        uint32_t start = getDriver().my_systick.get_ticks();
        while (i2c_->SR2 & I2C_SR2_BUSY) {
            if ((getDriver().my_systick.get_ticks() - start) > 3) {
                error_ = I2C_Error::ERR_I2C_BUS;
                state_ = I2C_State::BUSY;
                mode_  = I2C_Mode::NONE;
            }
        }
        RegisterUtils::clearBits(i2c_->CR1, I2C_CR1_SWRST);

        error_ = I2C_Error::NONE;
        state_ = I2C_State::READY;
        mode_  = I2C_Mode::NONE;
    }
}
