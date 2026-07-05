#pragma once

#include "FreeRTOS.h"
#include "projdefs.h"
#include "task.h"

enum class MonitoredTask : uint8_t { Led = 0, UartRx, Sht40, Stts2h, COUNT };

struct TaskHealth {
    std::atomic<TickType_t> lastCheckin{0};
    TickType_t              maxAllowedTicks;  // worst-case interval before considered "stuck"
};

// Indexed by MonitoredTask. Set max allowed interval generously above each
// task's own normal period (accounts for scheduling jitter, not just nominal rate).
static TaskHealth g_TaskHealth[static_cast<size_t>(MonitoredTask::COUNT)] = {
    /* Led */
    {{}, pdMS_TO_TICKS(300) }, // nominal 100ms
    /* UartRx */
    {{}, pdMS_TO_TICKS(30)  }, // nominal 10ms
    /* Sht40  */
    {{}, pdMS_TO_TICKS(800) }, // nominal 300ms
    /* Stts2h */
    {{}, pdMS_TO_TICKS(2000)}, // nominal 1000ms
};

inline void reportAlive(MonitoredTask id) {
    g_TaskHealth[static_cast<size_t>(id)].lastCheckin.store(xTaskGetTickCount(), std::memory_order_relaxed);
}

void ledTask(void* params);

void sht40_Task(void* params);

void stts2h_Task(void* params);

void sensorTask(void* params);

void uartTask(void* params);

void wwdgTask(void* params);