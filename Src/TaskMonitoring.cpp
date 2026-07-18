#include "TaskMonitoring.hpp"

#include <stdio.h>

#include <atomic>

#include "AppMode.hpp"
#include "PacketReceiver.hpp"
#include "Sht40ad1b.hpp"
#include "cli.hpp"
#include "oled_SSD1306.hpp"
#include "pch.hpp"
#include "portmacro.h"
#include "projdefs.h"
#include "stts2h.hpp"

extern Sht40ad1b temp_sensor;
extern Stts2h stts_temp;
extern Cli cmd;
extern std::atomic_bool g_cmd_complete;
extern std::atomic<AppMode> g_AppMode;

void ledTask(void *params)
{
	Drivers &g = getDrivers();
	TickType_t xLastWakeTime = xTaskGetTickCount();
	const TickType_t xPeriod = pdMS_TO_TICKS(100);
	while (1) {
		g.gpio_led.toggle();
		reportAlive(MonitoredTask::Led);

		char buffer[32];
		snprintf(
			buffer, sizeof(buffer), "AppMode: %s",
			namingTable[static_cast<uint8_t>(g_AppMode.load(std::memory_order_relaxed))]
				.name);
		g.disp.Show(buffer, 0, 0, 0);
		vTaskDelayUntil(&xLastWakeTime, xPeriod); // replaces g.timer.start(100)
	}
}

void sht40_Task(void *params)
{
	SemaphoreHandle_t mutex = static_cast<SemaphoreHandle_t>(params);
	Drivers &g = getDrivers();
	TickType_t xLastWakeTime = xTaskGetTickCount();
	static std::atomic_bool cmd_complete{true};

	while (1) {
		xSemaphoreTake(mutex, portMAX_DELAY);
		g.i2c1.complete_flag_ = &cmd_complete;
		g_cmd_complete.store(false, std::memory_order_relaxed);

		if (g_AppMode.load(std::memory_order_relaxed) != AppMode::CliMode) {
			temp_sensor.read();
		}

		while (!temp_sensor.isIdle()) {
			g.i2c1.processRx();
			temp_sensor.ProcessData();
			vTaskDelay(pdMS_TO_TICKS(10));
		}

		xSemaphoreGive(mutex);
		reportAlive(MonitoredTask::Sht40);
		std::string_view str = "SHT40: Temp:{}, Rh:{}";
		const AppMode curMode = g_AppMode.load(std::memory_order_relaxed);
		switch (curMode) {
		case AppMode::SendPacket: {
			PacketV2<Sht40ad1b::SensorData, PacketType::VERSION_1> sht40_data{
				temp_sensor.getValue()};
			g.uart2.send(sht40_data.raw(), sht40_data.size());
			break;
		}
		case AppMode::Console:
			if (stts_temp.getState() == Stts2h::SensorState::IDLE) {
				LOG_PRINT(str, temp_sensor.getValue().temperature,
					  temp_sensor.getValue().humidity);
			}
			break;
		case AppMode::CliMode:
			if (cmd.getState() == CliState::Completed) {
				LOG_PRINT(str, temp_sensor.getValue().temperature,
					  temp_sensor.getValue().humidity);
				cmd.setState(CliState::WaitingForInput);
			}
			break;
		case AppMode::COUNT:
			break;
		}

		char buffer[64];
		FloatIntExtraction temp_f = convertInt(temp_sensor.getValue().temperature);
		FloatIntExtraction Rh_f = convertInt(temp_sensor.getValue().humidity);
		snprintf(buffer, sizeof(buffer), "Temp:%02d.%02d", temp_f.Integer, temp_f.Decimal);
		g.disp.Show(buffer, 0, 8, 1);
		snprintf(buffer, sizeof(buffer), "Rh:%02d.%02d", Rh_f.Integer, Rh_f.Decimal);
		g.disp.Show(buffer, 0, 16, 2);
		vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(300));
	}
}

void stts2h_Task(void *params)
{
	SemaphoreHandle_t mutex = static_cast<SemaphoreHandle_t>(params);
	Drivers &g = getDrivers();
	TickType_t xLastWakeTime = xTaskGetTickCount();
	static std::atomic_bool cmd_complete{true};

	while (1) {
		xSemaphoreTake(mutex, portMAX_DELAY);

		g.i2c1.complete_flag_ = &cmd_complete;
		g_cmd_complete.store(false, std::memory_order_relaxed);

		if (g_AppMode.load(std::memory_order_relaxed) != AppMode::CliMode) {
			stts_temp.read();
		}

		while (!stts_temp.isIdle()) {
			g.i2c1.processRx();
			stts_temp.processData();
			vTaskDelay(pdMS_TO_TICKS(10));
		}

		xSemaphoreGive(mutex);
		reportAlive(MonitoredTask::Stts2h);
		const AppMode curMode = g_AppMode.load(std::memory_order_relaxed);
		switch (curMode) {
		case AppMode::SendPacket: {
			PacketV2<float_t, PacketType::VERSION_2> stts2h_data{stts_temp.getTemp()};
			g.uart2.send(stts2h_data.raw(), stts2h_data.size());
			break;
		}
		case AppMode::Console:
			if (stts_temp.getState() == Stts2h::SensorState::IDLE) {
				LOG_PRINT("STTS2H: Temp:{}", stts_temp.getTemp());
			}
			break;
		case AppMode::CliMode:
			if (cmd.getState() == CliState::Completed) {
				LOG_PRINT("STTS2H: Temp:{}", stts_temp.getTemp());
				cmd.setState(CliState::WaitingForInput);
			}
			break;

		case AppMode::COUNT:
			break;
		}

		vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(1000));
	}
}

// Task 1 — sensorTask: owns I2C + sensor reading + packet sending

void sensorTask(void *params)
{
	Drivers &g = getDrivers();
	SpscRingBuffer<I2CCommand, 4> cmd_queue;
	while (1) {
		/* Timer to keep firing read command when it is elapsed.*/
		if (g_AppMode.load(std::memory_order_relaxed) != AppMode::CliMode) {
			cmd_queue.push({[](void *ctx) { static_cast<Sht40ad1b *>(ctx)->read(); },
					&temp_sensor});
			cmd_queue.push({[](void *ctx) { static_cast<Stts2h *>(ctx)->read(); },
					&stts_temp});
		}

		/* Command Queue */
		I2CCommand icmd;
		while (cmd_queue.pop(icmd)) {
			g_cmd_complete.store(false, std::memory_order_relaxed);
			getDrivers().i2c1.complete_flag_ = &g_cmd_complete;
			icmd.fn(icmd.ctx); // Read Call

			bool sensor_done = false;
			while (!sensor_done) {
				g.i2c1.processRx();
				temp_sensor.ProcessData(); // State is not update
				stts_temp.processData();
				// Both sensors idle = current command fully complete:
				bool sht40_idle = temp_sensor.isIdle();
				bool stts2h_idle = stts_temp.isIdle();
				if (sht40_idle && stts2h_idle) {
					break;
				}
				// Done when complete_flag is true AND no more I2C activity:
				if (g_cmd_complete.load(std::memory_order_acquire)) {
					sensor_done = true;
				}
				vTaskDelay(pdMS_TO_TICKS(10)); // yield while polling
			}
		}

		if constexpr (kSensorEnable) {
			/* Send data packet to PC via Uart2 */
			if (g_AppMode.load(std::memory_order_relaxed) == AppMode::SendPacket) {
				PacketV2<Sht40ad1b::SensorData, PacketType::VERSION_1> sht40_data{
					temp_sensor.getValue()};
				g.uart2.send(sht40_data.raw(), sht40_data.size());
				PacketV2<float_t, PacketType::VERSION_2> stts2h_data{
					stts_temp.getTemp()};
				g.uart2.send(stts2h_data.raw(), stts2h_data.size());
			} else if (g_AppMode.load(std::memory_order_relaxed) == AppMode::Console) {
				if (temp_sensor.getState() == Sht40ad1b::SensorState::IDLE) {
					LOG_INFO("STH40: Temp:{}, Rh:{}",
						 temp_sensor.getValue().temperature,
						 temp_sensor.getValue().humidity);
				}
				if (stts_temp.getState() == Stts2h::SensorState::IDLE) {
					LOG_INFO("STTS2H: Temp:{}", stts_temp.getTemp());
				}
			} else if (g_AppMode.load(std::memory_order_relaxed) == AppMode::CliMode) {
				if (cmd.getState() == CliState::Completed) {
					LOG_INFO("STH40: Temp:{}, Rh:{}",
						 temp_sensor.getValue().temperature,
						 temp_sensor.getValue().humidity);
					cmd.setState(CliState::WaitingForInput);
				}
			}
		}
		vTaskDelay(pdMS_TO_TICKS(500)); // ← yield while waiting
	}
}

// Task 2 — uartTask: owns CLI input (only when kCliEnable = true)
void uartTask(void *params)
{
	/* Command Line Interface Input Processing */
	while (1) {
		if (g_AppMode.load(std::memory_order_relaxed) == AppMode::CliMode) {
			Drivers &g = getDrivers();
			g.uart2.processRx();
			if (cmd.getState() == CliState::WaitingForInput) {
				cmd.get_input();
				cmd.setState(CliState::Processing);
			}
		}
		reportAlive(MonitoredTask::UartRx);
		vTaskDelay(pdMS_TO_TICKS(10));
	}
}

void wwdgTask(void *params)
{
	Drivers &g = getDrivers();
	TickType_t xLastWakeTime = xTaskGetTickCount();
	// ~50ms ceiling at 42MHz APB1 (WDGTB=3, T=W=0x7F) — keep refresh period
	// well under that with margin for scheduling jitter.
	const TickType_t xPeriod = pdMS_TO_TICKS(15);

	// Prime checkin times at boot so we don't false-trigger before tasks run once.
	TickType_t now = xTaskGetTickCount();
	for (auto &h : g_TaskHealth) {
		h.lastCheckin.store(now, std::memory_order_relaxed);
	}

	while (1) {
		bool allHealthy = true;
		now = xTaskGetTickCount();

		for (size_t i = 0; i < static_cast<size_t>(MonitoredTask::COUNT); ++i) {
			TickType_t last =
				g_TaskHealth[i].lastCheckin.load(std::memory_order_relaxed);
			TickType_t elapsed = now - last; // TickType_t wraparound-safe if unsigned
			if (elapsed > g_TaskHealth[i].maxAllowedTicks) {
				allHealthy = false;
				break;
			}
		}

		if (allHealthy) {
			g.wwdg.resetCounter();
		}
		// else: skip refresh deliberately — let WWDG expire and reset the MCU.

		vTaskDelayUntil(&xLastWakeTime, xPeriod);
	}
}
