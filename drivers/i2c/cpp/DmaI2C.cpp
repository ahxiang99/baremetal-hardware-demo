#include "DmaI2C.hpp"

#include <atomic>
#include <cstddef>
#include <cstdint>

#include "RegisterUtils.hpp"
#include "Stm32I2C.hpp"
#include "cpp/Dma.hpp"
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
    onPostI2CInit();
    return true;
}

void DmaI2C::onPostI2CInit() {
    initTxDma();
    initRxDma();
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
            self->dma_complete_.store(true, std::memory_order_relaxed);
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
            self->dma_complete_.store(true, std::memory_order_relaxed);
            self->RxEventFlag.store(true, std::memory_order::release);
        },
        this);
}

bool DmaI2C::Write(uint16_t DevAddress, const uint8_t* pData, uint16_t Size, uint32_t Timeout) {
    if (state_.load(std::memory_order::relaxed) != I2C_State::READY) {
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

    std::copy(pData, pData + Size, dmaTxBuffer.data());
    DevAddr    = DevAddress;
    XferSize   = Size;
    XferLength = XferSize;
    state_.store(I2C_State::BUSY_TX, std::memory_order_release);
    mode_ = I2C_Mode::MASTER;
    error_.store(I2C_Error::NONE, std::memory_order_relaxed);
    dma_complete_.store(false, std::memory_order::relaxed);

    if (Size == 1) {
        dma_complete_ = true;
        disable_DMA_request();
        enableInterrupt();
        generateStartCondition();
    } else {
        enable_DMA_request_tx();
        hdmatx.StartDataStream(reinterpret_cast<uint32_t>(dmaTxBuffer.data()), (uint32_t)&i2c_->DR, Size);
        /* Get Acknowledgement from Slave */
        enableInterrupt();
        generateStartCondition();
    }

    return true;
}
bool DmaI2C::Read(uint16_t DevAddress, uint8_t* pData, uint16_t Size, uint32_t Timeout) {
    LOG_DEBUG("Read() called: state={} SR2={}", static_cast<uint32_t>(state_.load(std::memory_order_relaxed)), (Hex)i2c_->SR2);
    if (state_.load(std::memory_order_relaxed) != I2C_State::READY) {
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

    XferPtr    = pData;
    XferSize   = Size;
    XferLength = XferSize;
    DevAddr    = static_cast<uint8_t>(DevAddress);
    mode_      = I2C_Mode::MASTER;
    state_.store(I2C_State::BUSY_RX, std::memory_order::release);
    error_.store(I2C_Error::NONE, std::memory_order::relaxed);
    dma_complete_.store(false, std::memory_order::relaxed);
    RxEventFlag.store(false, std::memory_order::relaxed);
    rxDmaPending_.store(true, std::memory_order::release);

    enable_DMA_request_rx();
    enableAckBit();
    enableInterrupt();
    generateStartCondition();
    return true;
}

bool DmaI2C::MemWrite(uint16_t DevAddress, uint8_t MemAddr, uint8_t* pData, uint16_t Size, uint32_t Timeout) {
    if (state_.load(std::memory_order_relaxed) != I2C_State::READY) return false;
    if (isHardwareBusy(Timeout)) {
        error_ = I2C_Error::ERR_I2C_BUS;
        return false;
    }
    if (Size == 0 || pData == nullptr) {
        error_ = I2C_Error::ERR_I2C_DATA_EMPTY;
        return false;
    }
    DevAddr  = static_cast<uint8_t>(DevAddress);
    MemAddr_ = MemAddr;
    for (size_t i = 0; i < Size; i++) {
        TxBuffer.push(pData[i]);
    }
    XferSize   = Size;
    XferLength = XferSize;
    mode_      = I2C_Mode::MEM_WRITE;
    state_.store(I2C_State::BUSY_TX, std::memory_order_release);
    error_.store(I2C_Error::NONE, std::memory_order_relaxed);
    dma_complete_.store(true, std::memory_order_relaxed);  // pre-set: no DMA for mem addr byte

    disable_DMA_request();
    enableInterrupt();
    generateStartCondition();
    return true;
}

bool DmaI2C::MemRead(uint16_t DevAddress, uint8_t MemAddr, uint8_t* pData, uint16_t Size, uint32_t Timeout) {
    if (state_.load(std::memory_order_relaxed) != I2C_State::READY) return false;
    if (isHardwareBusy(Timeout)) {
        error_ = I2C_Error::ERR_I2C_BUS;
        return false;
    }
    if (Size == 0 || pData == nullptr) {
        error_ = I2C_Error::ERR_I2C_DATA_EMPTY;
        return false;
    }

    DevAddr  = static_cast<uint8_t>(DevAddress);
    MemAddr_ = MemAddr;
    TxBuffer.push(MemAddr);
    XferPtr    = pData;
    XferSize   = Size;
    XferLength = XferSize;

    mode_      = I2C_Mode::MEM_READ;
    state_.store(I2C_State::BUSY_TX, std::memory_order_release);
    error_.store(I2C_Error::NONE, std::memory_order_relaxed);
    dma_complete_.store(true, std::memory_order_relaxed);  // pre-set: no DMA for mem addr byte
    RxEventFlag.store(false, std::memory_order_relaxed);
    rxDmaPending_.store(false, std::memory_order_relaxed);

    disable_DMA_request();
    enableInterrupt();
    generateStartCondition();
    return true;
}

void DmaI2C::handleMasterEventInterrupt(const uint32_t& sr1, const uint32_t& sr2, const I2C_State& snap) {
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
            error_.store(I2C_Error::ERR_I2C_SB, std::memory_order::relaxed);
            state_.store(I2C_State::READY, std::memory_order::relaxed);
        }
        return;
    }

    // ── ADDR: Address sent and ACKed by slave ─────────────────────────────────
    // During TX: slave ACKed address, DMA is already running — just clear flag.
    // During RX: arm DMA first, THEN clear (releases clock stretch).
    if (sr1 & I2C_SR1_ADDR) {
        if (snap == I2C_State::BUSY_RX && rxDmaPending_.load(std::memory_order_acquire)) {
            rxDmaPending_.store(false, std::memory_order::relaxed);
            if (XferSize == 1) {
                // RM0368 §18.3.8: 1-byte DMA receive is broken — LAST sends NACK after the
                // DMA reads DR, but by then the slave was already ACK'd on byte 1's 9th clock
                // and begins sending byte 2, leaving RXNE=1 stuck and the bus hung.
                // Fix: abandon DMA, disable ACK now, queue STOP, read via RXNE interrupt.
                push_isr_event(I2C_IsrTag::EV_ADDR_RX_1BYTE);
                disable_DMA_request();
                disableAckBit();
                clearAddrFlag();                                     // release clock stretch before programming STOP
                generateStopCondition();                             // queued: fires after the single byte is received
                RegisterUtils::setBits(i2c_->CR2, I2C_CR2_ITBUFEN);  // arm RXNE interrupt
            } else {
                push_isr_event(I2C_IsrTag::EV_ADDR_RX, XferSize);
                hdmarx.StartDataStream(reinterpret_cast<uint32_t>(&i2c_->DR), reinterpret_cast<uint32_t>(rxBuffer.data()), XferSize);
                clearAddrFlag();  // must happen after DMA armed
            }
        } else {
            push_isr_event(I2C_IsrTag::EV_ADDR_TX);
            clearAddrFlag();
        }
        return;
    }

    // ── BTF: Byte Transfer Finished ───────────────────────────────────────────
    // This fires after DMA completes AND the last byte has been shifted out.
    // Only issue STOP here for TX, and only when DMA is actually done.
    if (sr1 & I2C_SR1_BTF) {
        if (snap == I2C_State::BUSY_TX) {
            if (dma_complete_.load(std::memory_order::relaxed) && XferSize == 1) {
                push_isr_event(I2C_IsrTag::EV_BTF_STOP);
                generateStopCondition();
                disableInterrupt();
                state_.store(I2C_State::READY, std::memory_order::relaxed);
                mode_ = I2C_Mode::NONE;
                dma_complete_.store(false, std::memory_order::relaxed);
            } else {
                // DMA TC hasn't fired yet — stall BTF by reading DR
                // This clears BTF without advancing the state machine
                push_isr_event(I2C_IsrTag::EV_BTF_STALL);
                volatile uint32_t dummy = i2c_->DR;
                (void)dummy;
            }
        } else if (snap == I2C_State::BUSY_RX) {
            if (dma_complete_.load(std::memory_order_relaxed)) {
                push_isr_event(I2C_IsrTag::EV_BTF_STOP);
                disableAckBit();
                generateStopCondition();
                disableInterrupt();
                state_.store(I2C_State::READY, std::memory_order::relaxed);
                mode_ = I2C_Mode::NONE;
                dma_complete_.store(false, std::memory_order_relaxed);
            } else {
                push_isr_event(I2C_IsrTag::EV_BTF_STALL);
                volatile uint32_t dummy = i2c_->DR;
                (void)dummy;
            }
        }
        return;
    }

    // ── RXNE: 1-byte interrupt-mode receive (DMA path disabled for XferSize==1) ─
    if (sr1 & I2C_SR1_RXNE) {
        if (snap == I2C_State::BUSY_RX && !rxDmaPending_.load(std::memory_order_acquire)) {
            push_isr_event(I2C_IsrTag::EV_RXNE_1BYTE, i2c_->SR1);
            rxBuffer[0] = static_cast<uint8_t>(i2c_->DR);  // STOP already queued in ADDR handler
            RegisterUtils::clearBits(i2c_->CR2, I2C_CR2_ITBUFEN);
            disableInterrupt();
            mode_ = I2C_Mode::NONE;
            dma_complete_.store(false, std::memory_order_relaxed);
            RxEventFlag.store(true, std::memory_order_release);
            state_.store(I2C_State::READY, std::memory_order_release);
        }
        return;
    }
    // ── TXE alone: DMA not done yet, do NOT generate STOP ────────────────────
    if (sr1 & I2C_SR1_TXE) {
        if (snap == I2C_State::BUSY_TX && dma_complete_) {
            // Single-byte path: write the byte directly on TXE
            // Only if DMA is not running (dma_complete_ pre-set to true)
            if (!(i2c_->CR2 & I2C_CR2_DMAEN) && XferSize == 1) {
                push_isr_event(I2C_IsrTag::EV_TXE, dmaTxBuffer[0]);
                i2c_->DR = dmaTxBuffer[0];
                RegisterUtils::clearBits(i2c_->CR2, I2C_CR2_ITBUFEN);
            }
        }
        return;
    }
}

void DmaI2C::handleMemEventInterrupt(const uint32_t& sr1, const uint32_t& sr2, const I2C_State& snap) {
    push_isr_event(I2C_IsrTag::EV_ENTRY, sr1, sr2);
    // ── SB: Start condition generated ────────────────────────────────────────
    if (sr1 & I2C_SR1_SB) {
        push_isr_event(I2C_IsrTag::EV_SB, static_cast<uint32_t>(snap), DevAddr);
        if (snap == I2C_State::BUSY_TX) {
            i2c_->DR = DevAddr & ~(1U << 0);  // write address
        } else if (snap == I2C_State::BUSY_RX) {
            i2c_->DR = DevAddr | (1U << 0);  // read address
        } else {
            generateStopCondition();
            error_.store(I2C_Error::ERR_I2C_SB, std::memory_order::relaxed);
            state_.store(I2C_State::READY, std::memory_order::relaxed);
        }
        return;
    }

    // ── ADDR: Address sent and ACKed by slave ─────────────────────────────────
    // During TX: slave ACKed address, DMA is already running — just clear flag.
    // During RX: arm DMA first, THEN clear (releases clock stretch).
    if (sr1 & I2C_SR1_ADDR) {
        if (snap == I2C_State::BUSY_RX && rxDmaPending_.load(std::memory_order_acquire)) {
            rxDmaPending_.store(false, std::memory_order::relaxed);
            if (XferLength == 1) {
                // RM0368 §18.3.8: 1-byte DMA receive is broken — LAST sends NACK after the
                // DMA reads DR, but by then the slave was already ACK'd on byte 1's 9th clock
                // and begins sending byte 2, leaving RXNE=1 stuck and the bus hung.
                // Fix: abandon DMA, disable ACK now, queue STOP, read via RXNE interrupt.
                push_isr_event(I2C_IsrTag::EV_ADDR_RX_1BYTE);
                disable_DMA_request();
                disableAckBit();
                clearAddrFlag();                                     // release clock stretch before programming STOP
                generateStopCondition();                             // queued: fires after the single byte is received
                RegisterUtils::setBits(i2c_->CR2, I2C_CR2_ITBUFEN);  // arm RXNE interrupt
            } else {
                push_isr_event(I2C_IsrTag::EV_ADDR_RX, XferLength);
                hdmarx.StartDataStream(reinterpret_cast<uint32_t>(&i2c_->DR), reinterpret_cast<uint32_t>(rxBuffer.data()), XferLength);
                clearAddrFlag();  // must happen after DMA armed
            }
        } else {
            push_isr_event(I2C_IsrTag::EV_ADDR_TX);
            RegisterUtils::setBits(i2c_->CR2, I2C_CR2_ITBUFEN);
            clearAddrFlag();
        }
        return;
    }

    // ── BTF: Byte Transfer Finished ───────────────────────────────────────────
    // This fires after DMA completes AND the last byte has been shifted out.
    // Only issue STOP here for TX, and only when DMA is actually done.
    if (sr1 & I2C_SR1_BTF) {
        if (snap == I2C_State::BUSY_TX) {
            if (dma_complete_.load(std::memory_order::relaxed)) {
                if (mode_ == I2C_Mode::MEM_READ) {
                    // MEM_READ: register address byte sent — issue repeated START to switch to RX
                    push_isr_event(I2C_IsrTag::EV_MEM_RESTART, MemAddr_);
                    state_.store(I2C_State::BUSY_RX, std::memory_order_relaxed);
                    dma_complete_.store(false, std::memory_order_relaxed);
                    rxDmaPending_.store(true, std::memory_order_release);
                    enable_DMA_request_rx();
                    enableAckBit();
                    generateStartCondition();  // repeated START — no STOP
                } else {
                    if (XferSize == 0) {
                        disableInterrupt();
                        generateStopCondition();
                        state_.store(I2C_State::READY, std::memory_order_relaxed);
                        mode_ = I2C_Mode::NONE;
                    } else {
                        uint8_t byte{0};
                        TxBuffer.pop(byte);
                        i2c_->DR = byte;
                        XferSize--;
                    }
                }
            } else {
                // DMA TC hasn't fired yet — stall BTF by reading DR
                // This clears BTF without advancing the state machine
                push_isr_event(I2C_IsrTag::EV_BTF_STALL);
                volatile uint32_t dummy = i2c_->DR;
                (void)dummy;
            }
        } else if (snap == I2C_State::BUSY_RX) {
            if (dma_complete_.load(std::memory_order_relaxed)) {
                push_isr_event(I2C_IsrTag::EV_BTF_STOP);
                disableAckBit();
                generateStopCondition();
                disableInterrupt();
                state_.store(I2C_State::READY, std::memory_order::relaxed);
                mode_ = I2C_Mode::NONE;
                dma_complete_.store(false, std::memory_order_relaxed);
            } else {
                push_isr_event(I2C_IsrTag::EV_BTF_STALL);
                volatile uint32_t dummy = i2c_->DR;
                (void)dummy;
            }
        }
        return;
    }
    // ── RXNE: 1-byte interrupt-mode receive (DMA path disabled for XferSize==1) ─
    if (sr1 & I2C_SR1_RXNE) {
        if (snap == I2C_State::BUSY_RX && !rxDmaPending_.load(std::memory_order_acquire)) {
            push_isr_event(I2C_IsrTag::EV_RXNE_1BYTE, i2c_->SR1);
            rxBuffer[0] = static_cast<uint8_t>(i2c_->DR);  // STOP already queued in ADDR handler
            RegisterUtils::clearBits(i2c_->CR2, I2C_CR2_ITBUFEN);
            disableInterrupt();
            mode_ = I2C_Mode::NONE;
            dma_complete_.store(false, std::memory_order_relaxed);
            RxEventFlag.store(true, std::memory_order_release);
            state_.store(I2C_State::READY, std::memory_order_release);
        }
        return;
    }

    // ── TXE alone: DMA not done yet, do NOT generate STOP ────────────────────
    if (sr1 & I2C_SR1_TXE) {
        if (snap == I2C_State::BUSY_TX && dma_complete_) {
            // Single-byte path: write the byte directly on TXE
            // Only if DMA is not running (dma_complete_ pre-set to true)
            if (!(i2c_->CR2 & I2C_CR2_DMAEN)) {
                push_isr_event(I2C_IsrTag::EV_MEM_ADDR, MemAddr_);
                i2c_->DR = MemAddr_;
                RegisterUtils::clearBits(i2c_->CR2, I2C_CR2_ITBUFEN);
            }
        }
        return;
    }
}

void DmaI2C::handleEVInterrupt() {
    const uint32_t  sr1  = i2c_->SR1;
    const uint32_t  sr2  = i2c_->SR2;  // reading SR2 also helps clear ADDR in some cases
    const I2C_State snap = state_.load(std::memory_order::relaxed);

    if (mode_ == I2C_Mode::MASTER) {
        handleMasterEventInterrupt(sr1, sr2, snap);
    } else if (mode_ == I2C_Mode::MEM_READ || mode_ == I2C_Mode::MEM_WRITE) {
        handleMemEventInterrupt(sr1, sr2, snap);
    }
}

void DmaI2C::handleERInterrupt() {
    const uint32_t sr1 = i2c_->SR1;

    // 1. Acknowledge Failure (AF) - The Sensor didn't respond
    if (sr1 & I2C_SR1_AF) {
        push_isr_event(I2C_IsrTag::ER_AF, sr1);
        clearAFFlag();
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

    // Release the bus and reset driver state so the next transaction can proceed.
    // Without generateStopCondition() here the bus stays stuck; without resetting
    // state_ to READY every subsequent Write/Read returns false permanently.
    generateStopCondition();
    RegisterUtils::clearBits(i2c_->CR2, I2C_CR2_DMAEN | I2C_CR2_LAST);
    RegisterUtils::clearBits(i2c_->CR2, I2C_CR2_ITEVTEN | I2C_CR2_ITBUFEN | I2C_CR2_ITERREN);
    disableAckBit();
    mode_ = I2C_Mode::NONE;
    dma_complete_.store(false, std::memory_order_relaxed);
    RxEventFlag.store(false, std::memory_order_relaxed);
    state_.store(I2C_State::READY, std::memory_order_release);
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
void DmaI2C::onDataReceived() {
    for (auto& c : receivers_) {
        c.notify();
    }
}
void DmaI2C::processRx() {
    drain_isr_log();
    if (RxEventFlag.load(std::memory_order_acquire) == false) return;
    RxEventFlag.store(false, std::memory_order_relaxed);

    if (dma_complete_.load(std::memory_order_relaxed) == true) {
        /* Rx Finally Completed */
        disableAckBit();
        generateStopCondition();
        disableInterrupt();
        state_.store(I2C_State::READY, std::memory_order::relaxed);
        mode_ = I2C_Mode::NONE;
    }

    /* Update Rx Buffer Head */
    for (size_t i = 0; i < XferLength; ++i) {
        XferPtr[i] = rxBuffer[i];
    }
    onDataReceived();
    if (complete_flag_) complete_flag_->store(true, std::memory_order_release);
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
            case I2C_IsrTag::EV_MEM_ADDR:
                LOG_DEBUG("MEM_READ TXE: sending reg addr {}", (Hex)e.a);
                break;
            case I2C_IsrTag::EV_MEM_RESTART:
                LOG_DEBUG("MEM_READ BTF: reg addr {} sent, issuing repeated START", (Hex)e.a);
                break;
            case I2C_IsrTag::EV_ADDR_RX_1BYTE:
                LOG_DEBUG("ADDR RX 1-byte: interrupt mode, ACK disabled, STOP queued");
                break;
            case I2C_IsrTag::EV_RXNE_1BYTE:
                LOG_DEBUG("RXNE 1-byte: read DR={}", (Hex)e.a);
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
