#include <atomic>
#include <cstdio>

#include "AppMode.hpp"
#include "FloatIntExtraction.hpp"
#include "MAX30102.hpp"
#include "cli.hpp"
#include "logger.hpp"
#include "RingBuffer.hpp"
#include "SensorPacket.hpp"
#include "Sht40ad1b.hpp"
#include "cpp/Dma.hpp"
#include "cpp/ExtiInput.hpp"
#include "cpp/Gpio.hpp"
#include "cpp/II2C.hpp"
#include "cpp/Stm32GpioPin.hpp"
#include "cpp/Stm32I2C.hpp"
#include "cpp/UartConcepts.hpp"
#include "cpp/UartRef.hpp"
#include "drivers.hpp"
#include "init.hpp"
#include "low-level/nvic.h"
#include "low-level/rcc_bitfields.h"
#include "low-level/syscfg_registers.h"
#include "pch.hpp"
#include "stts2h.hpp"

// Global Variables
Sht40ad1b temp_sensor(I2C_Ref::from(getDrivers().i2c1));
Stts2h stts_temp(I2C_Ref::from(getDrivers().i2c1));
Max30102 oxiSensor(I2C_Ref::from(getDrivers().i2c1));
Cli cmd;
std::atomic_bool g_cmd_complete{true};
std::atomic<AppMode> g_AppMode{AppMode::Console};

/* Function Prototype */
void register_fn_callback();
void appModeOperation(Drivers &, uint32_t &);
void oled_screen_update(Drivers &, uint32_t &);

void i2c_address_scan()
{
	constexpr i2c_config_t i2c_config{
		.DevNum = i2c_device_t::I2C_D1,
		.ClockFreq = i2c_freq_t::_100KHz,
		.OwnAddress1 = 0,
		.AddressingMode = i2c_addressmode_t::AddressMode_7Bit,
		.DualAddressMode = 0,
		.OwnAddress2 = 0,
	};
	Stm32I2C i2c;
	i2c.initialize(i2c_config);

	for (size_t i = 0x00; i <= 0x7F; i++) {
		const uint8_t data = 0;
		if (i2c.Write(i << 1, &data, 1, 3)) {
			LOG_INFO("Address: {} is valid", (Hex)i);
			break;
		} else {
			LOG_INFO("Address: {} is not valid", (Hex)i);
			RegisterUtils::clearBits(I2C1->SR1, 1 << 10);
			RegisterUtils::setBits(I2C1->CR1, 1 << 9);
		}
	}
}

/* Main Program Start Here */
int main()
{
	Drivers &g = getDrivers();
	initDriver(g);
	register_fn_callback();
	temp_sensor.initialize(Sht40ad1b::Command::HIGH_PRECISION);
	stts_temp.initialize();

	if (!g.timer.isRunning()) {
		g.timer.start(100);
	}

	SpscRingBuffer<I2CCommand, 4> cmd_queue;

	if constexpr (kOxiMeterEnable) {
		Max30102::SensorConfig oxi_config{Max30102::FifoSampleAvg::AVG4,
						  Max30102::FifoRollOver::ENABLE,
						  0,
						  Max30102::SensorMode::SpO2,
						  Max30102::SpO2ADC::SCALE_4096,
						  Max30102::SpO2SampleRate::RATE_100,
						  Max30102::SpO2PulseWidth::ADC_18BITS};
		oxiSensor.setConfig(oxi_config);

		if (!oxiSensor.getInit()) {
			oxiSensor.init();
			LOG_INFO("Part ID: {}", (uint16_t)oxiSensor.getPartID());
		}
	}

	uint32_t measure_start = g.my_systick.get_ticks();
	uint32_t disp_start = g.my_systick.get_ticks();
	uint32_t wwdg_refresh_start = g.my_systick.get_ticks();

	while (1) {
		const AppMode curMode = g_AppMode.load(std::memory_order_relaxed);

		g.i2c1.processRx();
		g.user_button.processEvent();

		/* Feed the watchdog on its own cadence, inside the ~29-50ms window the
		   configured prescaler/window/reload actually allows a refresh (the WWDG's
		   window is far shorter than the 100ms sensor-poll timer below, so it must
		   not be gated on that). Skipped while the sensor reports a fault so a
		   stuck I2C bus lets the WWDG time out and reset the board. */
		if (g.my_systick.get_ticks() - wwdg_refresh_start > 35) {
			if (temp_sensor.getFaultStatus().isOk()) {
				g.wwdg.resetCounter();
			}
			wwdg_refresh_start = g.my_systick.get_ticks();
		}

		/* Timer to keep firing read command when it is elapsed.*/
		if (g.timer.isElapsed()) {
			g.gpio_led.toggle();
			g.timer.start(100);

			if (curMode != AppMode::CliMode) {
				cmd_queue.push(
					{[](void *ctx) { static_cast<Sht40ad1b *>(ctx)->read(); },
					 &temp_sensor});
				cmd_queue.push(
					{[](void *ctx) { static_cast<Stts2h *>(ctx)->read(); },
					 &stts_temp});
			}
		}

		/* To process i2c and Sensor Data*/
		if constexpr (kSensorEnable) {
			temp_sensor.ProcessData();
			stts_temp.processData();

			/* Start Oximeter */
			if constexpr (kOxiMeterEnable) {
				oxiSensor.processData();
				cmd_queue.push(
					{[](void *ctx) { static_cast<Max30102 *>(ctx)->read(); },
					 &oxiSensor});
				char buf[128];
				if (oxiSensor.isDataReady()) {
					FloatIntExtraction SpO2_v =
						convertInt(oxiSensor.getData().spo2);
					snprintf(buf, sizeof(buf), "BPM and SpO2 Found.");
					g.disp.Show(buf, 0, 0, 0);
					snprintf(buf, sizeof(buf), "BPM: %d, SpO2: %d.%d",
						 oxiSensor.getData().bpm, SpO2_v.Integer,
						 SpO2_v.Decimal);
					g.disp.Show(buf, 0, 8, 1);
					oxiSensor.clearDataReadyFlag();
				}

				if (!oxiSensor.isFingerPresent()) {
					snprintf(buf, sizeof(buf), "Finger not Found.");
					g.disp.Show(buf, 0, 0, 0);
					g.disp.ClearPage(1);
					g.disp.FlushPage(1);
				}
			}
			/* End Oximeter */

			/* OLED Screen to update all the time. */
			oled_screen_update(g, disp_start);
			/* App Mode Changing */
			appModeOperation(g, measure_start);
		}

		/* Command Queue */
		if (g_cmd_complete.load(std::memory_order_acquire)) {
			I2CCommand cmd;
			if (cmd_queue.pop(cmd)) {
				g_cmd_complete.store(false, std::memory_order_relaxed);
				getDrivers().i2c1.complete_flag_ = &g_cmd_complete;
				cmd.fn(cmd.ctx);
			}
		}

		/* Sleep until the next interrupt (SysTick/DMA/UART/EXTI) instead of busy-spinning
		 */
		__WFI();
	}
	return 0;
}

/* Function Body */

void register_fn_callback()
{
	getDrivers().uart2.onDataReceived(
		[](void *ctx, const uint8_t *data, size_t len) {
			static_cast<Cli *>(ctx)->onUartData(data, len);
		},
		&cmd);
	cmd.setUart(UartRef::from(getDrivers().uart2));
	cmd.setSensor(&temp_sensor);

	if constexpr (kSensorEnable) {
		getDrivers().i2c1.addReceiver(temp_sensor);
		getDrivers().i2c1.addReceiver(stts_temp);
		getDrivers().i2c1.addReceiver(cmd);
		getDrivers().i2c1.addReceiver(oxiSensor);
	}

	getDrivers().user_button.setFnCallback(onButtonPress, &g_AppMode);
}

void appModeOperation(Drivers &g, uint32_t &measure_start)
{
	/* Send data packet to PC via Uart2 */
	const AppMode curMode = g_AppMode.load(std::memory_order_relaxed);
	switch (curMode) {
	case AppMode::Console: {
		if (g.my_systick.get_ticks() - measure_start > 1000) {
			LOG_INFO("SHT40: Temp: {} C, Rh: {}", temp_sensor.getValue().temperature,
				 temp_sensor.getValue().humidity);
			LOG_INFO("STTS2H: Temp: {}", stts_temp.getTemp());
			measure_start = g.my_systick.get_ticks();
		}
		break;
	}
	case AppMode::SendPacket: {
		static uint16_t seq = 0;
		if (g.my_systick.get_ticks() - measure_start > 350) {
			Env_Sensor_Data sht40_data{
				.last_rx_tick = static_cast<uint16_t>(g.my_systick.get_ticks() -
								      measure_start),
				.seq = seq++,
				.temp_x100 = static_cast<int16_t>(
					temp_sensor.getValue().temperature * 100),
				.rh_x100 =
					static_cast<int16_t>(temp_sensor.getValue().humidity * 100),
			};
			Packet<Env_Sensor_Data, PacketType::VERSION_0> pkt_sht40{sht40_data};
			Packet<float_t, PacketType::VERSION_2> stts2h_data{stts_temp.getTemp()};
			g.uart2.send({pkt_sht40.raw(), pkt_sht40.size()});
			g.uart2.send({stts2h_data.raw(), stts2h_data.size()});
			g.uart1.send({pkt_sht40.raw(), pkt_sht40.size()});
			measure_start = g.my_systick.get_ticks();
		}
		break;
	}
	case AppMode::CliMode: {
		/* Command Line Interface Input Processing */
		if (cmd.getState() == CliState::Completed) {
			LOG_INFO("STH40: Temp:{}, Rh:{}", temp_sensor.getValue().temperature,
				 temp_sensor.getValue().humidity);
			cmd.setState(CliState::WaitingForInput);
		}
		g.uart2.processRx();
		if (cmd.getState() == CliState::WaitingForInput) {
			cmd.get_input();
			cmd.setState(CliState::Processing);
		}
	}
	default:
		break;
	}
}

void oled_screen_update(Drivers &g, uint32_t &disp_start)
{
	char buf[128];

	/* Update Time */
	RTC_DateTypeDef d = g.rtc.getDate();
	RTC_TimeTypeDef t = g.rtc.getTime();
	snprintf(buf, sizeof(buf), "20%02d/%02d/%02d %02d:%02d:%02d", d.Year, d.Month, d.Day,
		 t.Hours, t.Minutes, t.Seconds);
	g.disp.Show(buf, 0, 0, 0);

	/* Update Mode */
	snprintf(buf, sizeof(buf), "AppMode: %s",
		 namingTable[static_cast<uint8_t>(g_AppMode.load(std::memory_order_relaxed))].name);
	g.disp.Show(buf, 0, 8, 1);

	/* Update Sensor Value */
	if (g.my_systick.get_ticks() - disp_start > 500) {
		FloatIntExtraction temp = convertInt(temp_sensor.getValue().temperature);
		snprintf(buf, sizeof(buf), "SHT40: Temp:%02d.%02d", temp.Integer, temp.Decimal);
		g.disp.Show(buf, 0, 16, 2);
		temp = convertInt(temp_sensor.getValue().humidity);
		snprintf(buf, sizeof(buf), "SHT40: Rh:%02d.%02d", temp.Integer, temp.Decimal);
		g.disp.Show(buf, 0, 24, 3);
		temp = convertInt(stts_temp.getTemp());
		snprintf(buf, sizeof(buf), "STTS2H: Temp:%02d.%02d", temp.Integer, temp.Decimal);
		g.disp.Show(buf, 0, 32, 4);
		disp_start = g.my_systick.get_ticks();
	}
}

/* Interrupt Handler Function Start Here*/

extern "C" void TIM3_IRQHandler(void)
{
	/* Clear Timer Interrupt */
	getDrivers().timer.handleInterrupt();
}

extern "C" void DMA1_Stream0_IRQHandler(void)
{
	getDrivers().i2c1.handleRxDmaInterrupt();
}

extern "C" void DMA1_Stream7_IRQHandler(void)
{
	getDrivers().i2c1.handleTxDmaInterrupt();
}

extern "C" void DMA1_Stream6_IRQHandler(void)
{
	// getDrivers().uart2.handleTxDmaInterrupt();
}

extern "C" void DMA1_Stream5_IRQHandler(void)
{
	// getDrivers().uart2.handleRxDmaInterrupt();
}

extern "C" void USART1_IRQHandler(void)
{
	getDrivers().uart1.handleInterrupt();
}

extern "C" void USART2_IRQHandler(void)
{
	getDrivers().uart2.handleInterrupt();
}

extern "C" void I2C1_EV_IRQHandler(void)
{
	getDrivers().i2c1.handleEVInterrupt();
}

extern "C" void I2C1_ER_IRQHandler(void)
{
	getDrivers().i2c1.handleERInterrupt();
}

extern "C" void EXTI9_5_IRQHandler(void)
{
}

extern "C" void EXTI15_10_IRQHandler(void)
{
	getDrivers().user_button.handleInterrupt();
}

extern "C" void HardFault_Handler(void)
{
	/* Put a breakpoint on the line below */
	volatile int loop = 1;
	while (loop) {
		// If the debugger stops here, a HardFault occurred!
	}
}
