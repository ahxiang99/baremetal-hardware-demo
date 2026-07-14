#include "Dma.hpp"

#include <cstdint>

#include "RegisterUtils.hpp"
#include "low-level/DMA_bitfields.h"
#include "low-level/rcc_bitfields.h"
#include "pch.hpp"

void IDma::enableInterrupt() {
    RegisterUtils::setBits(Instance->CR, DMA_SCR_TCIE | DMA_SCR_TEIE | DMA_SCR_DMEIE);
}

void IDma::disableInterrupt() {
    RegisterUtils::clearBits(Instance->CR, DMA_SCR_TCIE | DMA_SCR_TEIE | DMA_SCR_DMEIE);
}

void IDma::enableDMAclock() {
    volatile uint32_t* enrReg = &RCC->AHB1ENR;
    RegisterUtils::setBits(*enrReg, RCC_AHB1ENR_DMA1_EN);
}

void IDma::enableDMA() {
    clearFlag();
    RegisterUtils::setBits(Instance->CR, DMA_SCR_EN);
}

void IDma::disableDMA() {
    clearFlag();
    RegisterUtils::clearBits(Instance->CR, DMA_SCR_EN);
}

bool IDma::initialize() {
    /* 1. Enable Clock First before perform configuration */
    enableDMAclock();
    configureDMA();
    enableISR();
    return true;
}

void IDma::StartDataStream(uint32_t Src, uint32_t Dst, uint32_t len) {
    Prepare(Src, Dst, len);
    /* Enable DMA to start transmission */
    enableDMA();
}

void IDma::configureDMA() {
    uint32_t temp = Instance->CR;
    uint32_t mask = DMA_SCR_EN | (3 << DMA_SCR_DIR_Pos);
    RegisterUtils::clearBits(temp, mask);
    mask = (static_cast<uint8_t>(Config.Channel) << DMA_SCR_CHSEL_Pos) | (static_cast<uint8_t>(Config.Direction) << DMA_SCR_DIR_Pos) | (static_cast<uint8_t>(Config.Mode) << DMA_SCR_CIRC_Pos) |
           DMA_SCR_MINC;
    RegisterUtils::setBits(temp, mask);
    Instance->CR = temp;

    if (Config.Stream >= DMA_Stream::Stream_0 && Config.Stream <= DMA_Stream::Stream_3) {
        ISR  = &Parent->LISR;
        IFCR = &Parent->LIFCR;
    } else {
        ISR  = &Parent->HISR;
        IFCR = &Parent->HIFCR;
    }
    static const uint8_t flagBitshiftOffset[8U] = {0U, 6U, 16U, 22U, 0U, 6U, 16U, 22U};
    streamIdx                                   = flagBitshiftOffset[static_cast<uint8_t>(Config.Stream)];
}

void IDma::handleInterrupt() {
    if (*ISR & (DMA_LISR_TCIF0 << streamIdx)) {
        /* Transfer Complete Call Back*/
        clearFlag();
        disableInterrupt();
        disableDMA();
        if (cplt_fn_) cplt_fn_(cplt_ctx_);
    }
}

void IDma::setConfig(const DMA_Config& config) {
    Config   = config;
    Parent   = (config.Device == DMA_Device::DMA_D1) ? DMA1 : DMA2;
    Instance = &Parent->SMx[static_cast<uint8_t>(Config.Stream)];
}

void IDma::clearFlag() {
    RegisterUtils::setBits(*IFCR, 0x3F << streamIdx);
}
void IDma::setXferCpltCallback(DmaCpltFn fn, void* ctx) {
    cplt_fn_  = fn;
    cplt_ctx_ = ctx;
}
bool IDma::is_Enabled() const {
    return Instance->CR & DMA_SCR_EN;
}
DMA_Stream_TypeDef* IDma::getDmaStream() const {
    return Instance;
}
void IDma::Prepare(uint32_t Src, uint32_t Dst, uint32_t len) {
    /* Prepare Data Transfer Parameters for DMA */
    Instance->NDTR = len;
    if (Config.Direction == DMA_Direction::DMA_MEMORY_TO_PERIPH) {
        Instance->M0AR = Src;
        Instance->PAR  = Dst;
    } else if (Config.Direction == DMA_Direction::DMA_PERIPH_TO_MEMORY) {
        Instance->M0AR = Dst;
        Instance->PAR  = Src;
    }
    /* Enable Interrupt */
    enableInterrupt();
}
void IDma::enableStream() {
    enableDMA();
}
void IDma::disableStream() {
    disableDMA();
}
void IDma::enableISR() {
    if (Parent == DMA1) {
        switch (Config.Stream) {
            case DMA_Stream::Stream_0:
                My_NVIC_EnableIRQ(DMA1_Stream0_IRQn);
                break;
            case DMA_Stream::Stream_1:
                My_NVIC_EnableIRQ(DMA1_Stream1_IRQn);
                break;
            case DMA_Stream::Stream_2:
                My_NVIC_EnableIRQ(DMA1_Stream2_IRQn);
                break;
            case DMA_Stream::Stream_3:
                My_NVIC_EnableIRQ(DMA1_Stream3_IRQn);
                break;
            case DMA_Stream::Stream_4:
                My_NVIC_EnableIRQ(DMA1_Stream4_IRQn);
                break;
            case DMA_Stream::Stream_5:
                My_NVIC_EnableIRQ(DMA1_Stream5_IRQn);
                break;
            case DMA_Stream::Stream_6:
                My_NVIC_EnableIRQ(DMA1_Stream6_IRQn);
                break;
            case DMA_Stream::Stream_7:
                My_NVIC_EnableIRQ(DMA1_Stream7_IRQn);
                break;
        }
    } else {
        switch (Config.Stream) {
            case DMA_Stream::Stream_0:
                My_NVIC_EnableIRQ(DMA2_Stream0_IRQn);
                break;
            case DMA_Stream::Stream_1:
                My_NVIC_EnableIRQ(DMA2_Stream1_IRQn);
                break;
            case DMA_Stream::Stream_2:
                My_NVIC_EnableIRQ(DMA2_Stream2_IRQn);
                break;
            case DMA_Stream::Stream_3:
                My_NVIC_EnableIRQ(DMA2_Stream3_IRQn);
                break;
            case DMA_Stream::Stream_4:
                My_NVIC_EnableIRQ(DMA2_Stream4_IRQn);
                break;
            case DMA_Stream::Stream_5:
                My_NVIC_EnableIRQ(DMA2_Stream5_IRQn);
                break;
            case DMA_Stream::Stream_6:
                My_NVIC_EnableIRQ(DMA2_Stream6_IRQn);
                break;
            case DMA_Stream::Stream_7:
                My_NVIC_EnableIRQ(DMA2_Stream7_IRQn);
                break;
        }
    }
}
