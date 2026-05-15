#include <math.h>
#include <stddef.h>

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iterator>
#include <string_view>

#include "TempSensor.hpp"
#include "bit_utils.h"
#include "drivers/gpio/cpp/gpio.hpp"
#include "drivers/i2c/cpp/i2c.hpp"
#include "drivers/systick/cpp/systick.hpp"
#include "drivers/timer/low-level/tim.h"
#include "drivers/uart/cpp/uart.hpp"

MySysTick timer;

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

GPIO            GPIO_CTL_A;
I2C_InitTypeDef Config{I2C_1, I2C_SPEED_STANDARD, 0, 0, 0, 0};
i2c_device      p_hI2C1(&Config);
TempSensor      Sensor_t(p_hI2C1);
volatile bool   trigger_measurement = false;

int             main() {
    // Enable FPU by setting bits 20, 21, 22, and 23
    CPACR |= ((3UL << 20) | (3UL << 22));

    // Manual Barrier        instructions(Assembly)
    __asm volatile("dsb 0xf" ::: "memory");
    __asm volatile("isb 0xf" ::: "memory");

    USART_InitTypeDef uart_cfg{USART_D2, RX_TX, _9600, USART_CR1_RXNEIE};
    UARTDevice        UART2{&uart_cfg};

    GPIO_InitTypeDef  cfg_i2C{GPIO_PB, GPIO_PIN_8 | GPIO_PIN_9, GPIO_MODE_ALTFN, GPIO_OTYPER_OD, GPIO_OSPEEDR_LS, GPIO_PUPDR_PULLUP, 0x04};
    GPIO              GPIO_CTL_B(&cfg_i2C);

    GPIO_InitTypeDef  cfg{GPIO_PA, GPIO_PIN_5, GPIO_MODE_OUTPUT, GPIO_OTYPER_PP, GPIO_OSPEEDR_LS, GPIO_PUPDR_NOPULL, 0};
    GPIO_CTL_A.InitDriver(&cfg);
    timer.init();
    timer.delay_ms(10);

    char command[64] = {"Booting...\r\n"};
    UART2.Print(command, sizeof(command));

    // I2C_InitTypeDef Config{I2C_1, I2C_SPEED_STANDARD, 0, 0, 0, 0};
    // i2c_device      p_hI2C1(&Config);

    // Sensor_t.AssignDriver(p_hI2C1);

    TIM_InitTypeDef Tim_Config{APB1_TIMER_3, 999, 15999, TIM_COUNTERMODE_UP, TIM_CLOCKDIVISION_DIV1, TIM_AUTORELOAD_PRELOAD_ENABLE};
    TIM_TypeDef*    Timer = TIM3;
    TIM_Init(Timer, &Tim_Config);
    while (1) {
        if (trigger_measurement) {
            Sensor_t.StartConversation();
            trigger_measurement = false;
        }
        Sensor_t.Process();
        float_t temp = Sensor_t.getValue();
        if (temp != 0.0f && trigger_measurement) {
            FloatIntExtraction result = convertInt(temp);
            snprintf(command, sizeof(command), "Reading: %02d.%02d\r\n", result.Integer, result.Decimal);
            UART2.Print(command, strlen(command));
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
