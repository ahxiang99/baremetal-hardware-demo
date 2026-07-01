#include "FreeRTOS.h"
#include "task.h"  // ← provides task-related declarations

extern "C" {
void vApplicationStackOverflowHook(TaskHandle_t xTask, char* pcTaskName) {
    (void)xTask;
    while (1);
}

void vApplicationIdleHook(void) {
    // optional — called by idle task
}

void vApplicationMallocFailedHook(void) {
    while (1);
}
}