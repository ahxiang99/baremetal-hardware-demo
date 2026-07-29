#pragma once

#include <atomic>
#include <cstdint>
#include <cstdio>

#include "RingBuffer.hpp"
#include "SensorRef.hpp"
#include "cpp/Dma.hpp"
#include "cpp/Stm32I2C.hpp"
#include "drivers.hpp"

struct XferParams {
	uint32_t DevAddr;
	uint32_t XferSize;
};

// Tags for the ISR-safe diagnostic log — written in ISR, consumed in
// processRx()
enum class I2C_IsrTag : uint8_t {
	EV_ENTRY,
	EV_SB,
	EV_ADDR_TX,
	EV_ADDR_RX,
	EV_ADDR_RX_1BYTE,
	EV_BTF_STOP,
	EV_BTF_STALL,
	EV_TXE,
	EV_MEM_ADDR,
	EV_MEM_RESTART,
	EV_RXNE_1BYTE,
	ER_AF,
	ER_BERR,
	ER_ARLO,
	ER_OVR,
	TX_DMA_FIRED,
	TX_DMA_DONE,
	RX_DMA_DONE
};

class DmaI2C: public Stm32I2C
{
      public:
	Result<> initialize(const i2c_config_t &cfg, const DMA_Config &hdmatx_cfg,
			    const DMA_Config &hdmarx_cfg);
	bool Write(uint16_t DevAddress, const uint8_t *pData, uint16_t Size, uint32_t Timeout);
	bool Read(uint16_t DevAddress, uint8_t *pData, uint16_t Size, uint32_t Timeout);
	bool MemRead(uint16_t DevAddress, uint8_t MemAddr, uint8_t *pData, uint16_t Size,
		     uint32_t Timeout);
	bool MemWrite(uint16_t DevAddress, uint8_t MemAddr, uint8_t *pData, uint16_t Size,
		      uint32_t Timeout);
	void handleEVInterrupt();
	void handleERInterrupt();
	void handleTxDmaInterrupt();
	void handleRxDmaInterrupt();
	void onDataReceived();
	void processRx();

	template <Sensor T> void addReceiver(T &obj)
	{
		if (receivers_count < receivers_.size()) {
			receivers_[receivers_count++] = SensorRef(obj);
		}
	}

	std::atomic<bool> *complete_flag_{nullptr};

      protected:
	void onPostI2CInit();
	IDma hdmatx;
	IDma hdmarx;

      private:
	void initTxDma();
	void initRxDma();

	bool isHardwareBusy(const uint32_t &Timeout);

	void enable_DMA_request_tx();
	void enable_DMA_request_rx();
	void disable_DMA_request();

	void enableInterrupt();
	void disableInterrupt();

	void handleMasterEventInterrupt(const uint32_t &sr1, const uint32_t &sr2,
					const i2c_state_t &snap_state);
	void handleMemEventInterrupt(const uint32_t &sr1, const uint32_t &sr2,
				     const i2c_state_t &snap_state);

	std::array<uint8_t, BUFF_SIZE> dmaTxBuffer;
	std::array<uint8_t, BUFF_SIZE> rxBuffer;
	SpscRingBuffer<uint8_t, 16> TxBuffer;

	std::array<SensorRef, 8> receivers_;
	uint8_t receivers_count{0};

	uint8_t DevAddr;
	uint8_t MemAddr_;
	uint8_t *XferPtr;
	uint8_t XferSize;
	uint8_t XferLength;
	std::atomic_bool RxEventFlag{false};
	std::atomic_bool dma_complete_{false};
	std::atomic_bool rxDmaPending_{false};

	// ISR-safe diagnostic log: push_isr_event() is safe to call from any ISR;
	// drain_isr_log() must only be called from the main loop (processRx).
	void push_isr_event(I2C_IsrTag tag, uint32_t a = 0, uint32_t b = 0);
	void drain_isr_log();
#ifndef NDEBUG
	struct IsrEvent {
		I2C_IsrTag tag;
		uint32_t a;
		uint32_t b;
	};
	static constexpr size_t kIsrLogSize = 16;
	std::array<IsrEvent, kIsrLogSize> isr_log_ = {};
	volatile uint8_t isr_log_head_ = 0;
	uint8_t isr_log_tail_ = 0;
#endif
};
