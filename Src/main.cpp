#include <math.h>
#include <stddef.h>

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iterator>
#include <string_view>

#include "Middleware/logger.hpp"
#include "SHT4X.hpp"
#include "Sensor.hpp"
#include "bit_utils.h"
#include "drivers/gpio/cpp/gpio.hpp"
#include "drivers/i2c/cpp/i2c.hpp"
#include "drivers/systick/cpp/systick.hpp"
#include "drivers/timer/low-level/tim.h"
#include "drivers/uart/cpp/uart.hpp"

#define CPACR (*((volatile uint32_t*)0xE000ED88))

struct FloatIntExtraction {
    uint16_t Integer;
    uint16_t Decimal;
};

FloatIntExtraction convertInt(float_t value) {
    FloatIntExtraction result = {0, 0};
    result.Integer            = (uint16_t)value;
    result.Decimal            = (uint16_t)((value - (float)result.Integer) * 100.0f);
    return result;
}

// Global Variables
volatile bool trigger_measurement = false;
MySysTick     timer;

GPIO          GPIO_CTL_A;
GPIO          GPIO_CTL_B;
UARTDevice    UART2;
i2c_device    hi2c1;
SHT4X         Sensor_t;

void          Init_All_Driver() {
    Logger::set_level(LogLevel::DBG);

    USART_InitTypeDef uart_cfg{USART_D2, RX_TX, _9600};
    UART2.InitDriver(&uart_cfg);
    LOG_DEBUG("Booting...");
    LOG_DEBUG("USART2 Initialized");

    GPIO_InitTypeDef cfg_i2C{GPIO_PB, GPIO_PIN_8 | GPIO_PIN_9, GPIO_MODE_ALTFN, GPIO_OTYPER_OD, GPIO_OSPEEDR_LS, GPIO_PUPDR_PULLUP, 0x04};
    GPIO_CTL_B.InitDriver(&cfg_i2C);
    LOG_DEBUG("GPIOB Initialized");

    I2C_InitTypeDef i2c1_Config{I2C_1, I2C_SPEED_STANDARD, 0, 0, 0, 0};
    hi2c1.InitDriver(&i2c1_Config);
    LOG_DEBUG("I2C1 Initialized");

    // To Blink LED
    GPIO_InitTypeDef cfg{GPIO_PA, GPIO_PIN_5, GPIO_MODE_OUTPUT, GPIO_OTYPER_PP, GPIO_OSPEEDR_LS, GPIO_PUPDR_NOPULL, 0};
    GPIO_CTL_A.InitDriver(&cfg);
    LOG_DEBUG("GPIOA Initialized");

    LOG_DEBUG("Initialized Done...");
}

int main() {
    // Enable FPU by setting bits 20, 21, 22, and 23
    CPACR |= ((3UL << 20) | (3UL << 22));

    // Manual Barrier        instructions(Assembly)
    __asm volatile("dsb 0xf" ::: "memory");
    __asm volatile("isb 0xf" ::: "memory");

    Init_All_Driver();

    timer.init();

    TIM_InitTypeDef Tim_Config{APB1_TIMER_3, 999, 15999, TIM_COUNTERMODE_UP, TIM_CLOCKDIVISION_DIV1, TIM_AUTORELOAD_PRELOAD_ENABLE};
    TIM_TypeDef*    Timer = TIM3;

    TIM_Init(Timer, &Tim_Config);

    Sensor_t.Init(hi2c1, 0x88, "SHT40AD1B");

    while (1) {
        if (trigger_measurement) {
            Sensor_t.SetState(SensorState::MEASURING);
            Sensor_t.StartRead_IT();
            trigger_measurement = false;
        }

        timer.delay_ms(10);
        Sensor_t.StartRead_IT();
        // Read and Print Data to Console
        float_t temp = Sensor_t.getTemp();
        float_t rh   = Sensor_t.getRh();
        if (temp != 0.0f && trigger_measurement) {
            FloatIntExtraction result_temp = convertInt(temp);
            LOG_INFO("Temp Reading: {}.{}", result_temp.Integer, result_temp.Decimal);
        }
        if (rh != 0.0f && trigger_measurement) {
            FloatIntExtraction result_rh = convertInt(rh);
            LOG_INFO("RH Reading: {}.{}", result_rh.Integer, result_rh.Decimal);
        }
    }
    return 0;
}

void TIM3_IRQHandler(void) {
    if (TIM3->SR & TIM_SR_UIF) {
        CLEAR_BIT(TIM3->SR, TIM_SR_UIF);
        // Your code here (e.g., toggle an LED)
        GPIO_CTL_A.TogglePin(GPIO_PIN_5);
        trigger_measurement = true;
    }
}
