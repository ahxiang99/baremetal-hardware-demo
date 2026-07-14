#include "Sht40ad1b.hpp"
#include "TaskMonitoring.hpp"
#include "cli.hpp"
#include "cpp/Stm32GpioPin.hpp"
#include "cpp/Stm32I2C.hpp"
#include "cpp/Stm32Spi.hpp"
#include "init.hpp"
#include "oled_SSD1306.hpp"
#include "pch.hpp"
#include "semphr.h"
#include "stts2h.hpp"

// Global Variables
Sht40ad1b            temp_sensor(I2C_Ref::from(getDrivers().i2c1), "SHT40");
Stts2h               stts_temp(I2C_Ref::from(getDrivers().i2c1), Stts2h::SensorMode::ONE_SHOT);
Cli                  cmd;
std::atomic_bool     g_cmd_complete{true};
std::atomic<AppMode> g_AppMode{AppMode::Console};

/* Function Prototype */
void SetNVICPriority();
void RegisterCallback();

/* Main Program Start Here */
int main() {
    Drivers& g = getDrivers();
    initDriver(g);

    RegisterCallback();
    stts_temp.initialize();

    SemaphoreHandle_t i2c_mutex = xSemaphoreCreateMutex();
    if (i2c_mutex == NULL) {
        // Heap exhausted — can't create mutex
        __asm volatile("bkpt #0");
        while (1);
    }

    xTaskCreate(wwdgTask, "WWDG", 512, NULL, 5, NULL);

    xTaskCreate(ledTask, "LED", 1024, NULL, 4, NULL);

    xTaskCreate(uartTask, "UART", 256, NULL, 3, NULL);

    xTaskCreate(sht40_Task, "Sht40", 1024, i2c_mutex, 2, NULL);

    xTaskCreate(stts2h_Task, "Stts2h", 1024, i2c_mutex, 1, NULL);

    vTaskStartScheduler();

    while (1);  // heap exhaustion
    return 0;
}

/* Function Body */

void RegisterCallback() {
    getDrivers().uart2.onDataReceived([](void* ctx, const uint8_t* data, size_t len) { static_cast<Cli*>(ctx)->onUartData(data, len); }, &cmd);
    cmd.setUart(UartRef::from(getDrivers().uart2));
    cmd.setSensor(&temp_sensor);

    if constexpr (kSensorEnable) {
        getDrivers().i2c1.addReceiver(temp_sensor);
        getDrivers().i2c1.addReceiver(stts_temp);
        getDrivers().i2c1.addReceiver(cmd);
    }

    getDrivers().user_button.setFnCallback(onButtonPress, &g_AppMode);
}

void SetNVICPriority() {
    My_NVIC_SetPriority(DMA1_Stream0_IRQn, 6);  // I2C RX DMA
    My_NVIC_SetPriority(DMA1_Stream7_IRQn, 6);  // I2C TX DMA
    My_NVIC_SetPriority(DMA1_Stream6_IRQn, 6);  // UART TX DMA
    My_NVIC_SetPriority(DMA1_Stream5_IRQn, 6);  // UART RX DMA
    My_NVIC_SetPriority(USART2_IRQn, 6);        // UART
    My_NVIC_SetPriority(I2C1_EV_IRQn, 6);       // I2C event
    My_NVIC_SetPriority(I2C1_ER_IRQn, 6);       // I2C error
    My_NVIC_SetPriority(TIM3_IRQn, 6);          // Timer
    My_NVIC_SetPriority(EXTI15_10_IRQn, 7);     // Button
}

/* Interrupt Handler Function Start Here*/

extern "C" void TIM3_IRQHandler(void) {
    getDrivers().timer.handleInterrupt();
}

extern "C" void DMA1_Stream0_IRQHandler(void) {
    getDrivers().i2c1.handleRxDmaInterrupt();
}

extern "C" void DMA1_Stream7_IRQHandler(void) {
    getDrivers().i2c1.handleTxDmaInterrupt();
}

extern "C" void DMA1_Stream6_IRQHandler(void) {
    getDrivers().uart2.handleTxDmaInterrupt();
}

extern "C" void DMA1_Stream5_IRQHandler(void) {
    getDrivers().uart2.handleRxDmaInterrupt();
}

extern "C" void USART2_IRQHandler(void) {
    getDrivers().uart2.handleInterrupt();
}

extern "C" void I2C1_EV_IRQHandler(void) {
    getDrivers().i2c1.handleEVInterrupt();
}

extern "C" void I2C1_ER_IRQHandler(void) {
    getDrivers().i2c1.handleERInterrupt();
}

extern "C" void EXTI15_10_IRQHandler(void) {
    getDrivers().user_button.handleInterrupt();
}

extern "C" void HardFault_Handler(void) {
    // SCB registers — Cortex-M4 System Control Block
    volatile uint32_t hfsr = *reinterpret_cast<volatile uint32_t*>(0xE000ED2C);  // Hard Fault Status
    volatile uint32_t cfsr = *reinterpret_cast<volatile uint32_t*>(0xE000ED28);  // Configurable Fault Status
    volatile uint32_t bfar = *reinterpret_cast<volatile uint32_t*>(0xE000ED38);  // Bus Fault Address
    volatile uint32_t mmar = *reinterpret_cast<volatile uint32_t*>(0xE000ED34);  // MemManage Fault Address
    volatile uint32_t lr   = 0;
    __asm volatile("mov %0, lr" : "=r"(lr));  // link register — tells which context
    (void)hfsr;
    (void)cfsr;
    (void)bfar;
    (void)mmar;
    volatile int loop = 1;
    while (loop) {
    }
}