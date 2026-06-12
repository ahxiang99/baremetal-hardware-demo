#pragma once

#include <array>

#include "low-level/DMA_bitfields.h"
#include "low-level/DMA_registers.h"
#include "low-level/DMA_types.h"

enum class DMA_Channel : uint8_t { Channel_0, Channel_1, Channel_2, Channel_3, Channel_4, Channel_5, Channel_6, Channel_7 };
enum class DMA_Stream : uint8_t { Stream_0, Stream_1, Stream_2, Stream_3, Stream_4, Stream_5, Stream_6, Stream_7 };
enum class DMA_Direction : uint8_t { DMA_PERIPH_TO_MEMORY, DMA_MEMORY_TO_PERIPH, DMA_MEMORY_TO_MEMORY };
enum class DMA_Mode : uint8_t { Normal, Circular };

struct DMA_Config {
    DMA_TypeDef*  DMA_BaseAddress;
    DMA_Stream    Stream;
    DMA_Channel   Channel;
    DMA_Direction Direction;
    DMA_Mode      Mode;
};

class IDma {
   public:
    void setConfig(const DMA_Config& config);
    void StartDataStream(uint32_t Src, uint32_t Dst, uint32_t len);
    bool initialize();
    void handleInterrupt();
    bool is_Enabled() const;
    void Prepare(uint32_t Src, uint32_t Dst, uint32_t len);
    void enableStream();
    void disableStream();

    using DmaCpltFn = void (*)(void*);
    void                setXferCpltCallback(DmaCpltFn fn, void* ctx);

    DMA_Stream_TypeDef* getDmaStream() const;

   protected:
    DMA_TypeDef*        Parent;    // DMA1 or DMA2 Address
    DMA_Stream_TypeDef* Instance;  // Individual Stream Controller.
    DMA_Config          Config;
    volatile uint32_t*  ISR;
    volatile uint32_t*  IFCR;
    uint32_t            streamIdx;

   private:
    /* Variables */
    std::array<uint8_t, DMA_CHUNK_SIZE> dmaTxBuffer;
    /* Enable or Disable Function*/
    void enableDMAclock();
    void enableISR();

    void enableDMA();
    void disableDMA();

    void enableInterrupt();
    void disableInterrupt();

    /* Configuration Function */
    void loadBuffer(uint8_t* pData, size_t len);
    void configureDMA();

    /* Interrupt Event Handle Function */
    void clearFlag();

    /* Function Callback Ptr */
    DmaCpltFn cplt_fn_  = nullptr;
    void*     cplt_ctx_ = nullptr;
};