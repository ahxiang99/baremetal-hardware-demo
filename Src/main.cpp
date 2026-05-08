#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "Inc/libs/my_stm32f4_gpio_lib.h"
#include "Inc/libs/my_stm32f4_i2c_lib.h"
#include "Inc/libs/my_stm32f4_systick_lib.h"
#include "Inc/libs/my_stm32f4_uart_lib.h"
#include "my_stm32f4_systick_driver.h"

#define CPACR (*((volatile uint32_t*)0xE000ED88))

typedef struct {
    uint32_t dev_addr;
    uint32_t reg_addr;
    uint8_t  data[6];
} Sensor_TypeDef;

struct FloatIntExtraction {
    uint16_t Integer;
    uint16_t Decimal;
};

FloatIntExtraction convertInt(float value) {
    FloatIntExtraction result = {0, 0};
    result.Integer            = (uint16_t)value;
    result.Decimal            = (uint16_t)((value - (float)result.Integer) * 100.0f);
    return result;
}

static uint8_t crc_calculate(const uint8_t* data, uint16_t count) {
    const uint8_t crc8_polynomial = 0x31;
    uint8_t       crc             = 0xFF;

    /* Calculate 8-bit checksum for given polynomial */
    for (uint16_t index = 0; index < count; index++) {
        crc ^= data[index];
        for (uint8_t crc_bit = 8U; crc_bit > 0U; crc_bit--) {
            crc = ((crc & 0x80U) != 0U) ? ((crc << 1) ^ crc8_polynomial) : (crc << 1);
        }
    }

    return crc;
}

static uint8_t crc_check(const uint8_t* data, uint16_t count, uint8_t crc) {
    return (crc_calculate(data, count) == crc) ? 1U : 0U;
}

int main() {
    // Enable FPU by setting bits 20, 21, 22, and 23
    CPACR |= ((3UL << 20) | (3UL << 22));

    // Manual Barrier instructions (Assembly)
    __asm volatile("dsb 0xf" ::: "memory");
    __asm volatile("isb 0xf" ::: "memory");

    UARTComm uart(TX_ONLY, _9600);
    uart.LIB_UART_Init();
    char buffer[256] = "Booting\r\n";
    uart.LIB_UART_Write(buffer, sizeof(buffer));
    GPIO_InitTypeDef my_ledInitStruct = {GPIO_PIN_5, GPIO_MODE_OUTPUT, GPIO_NOPULL, 0, 0};
    Gpio             led(GPIOA_PORT, GPIOA, &my_ledInitStruct);

    I2C_InitTypeDef  i2c_initStruct{100000, I2C_DUTYCYCLE_2, 0, 0, 0, 0};
    Sensor_TypeDef   Temp_Sensor{0x44, 0x00, 0, 6};
    I2C              i2c_1{I2C1, &i2c_initStruct};

    while (1) {
        led.LIB_GPIO_TogglePin(GPIOA, GPIO_PIN_5);  // Toggle the LED
        MySysTick(1000);                            // Wait for 1 second

        uint8_t cmd = 0xFD;
        i2c_1.LIB_I2C_TRANSMIT(Temp_Sensor.dev_addr, &cmd, 1);
        MySysTick(100);

        i2c_1.LIB_I2C_READ_REGISTER(Temp_Sensor.dev_addr, (uint8_t*)Temp_Sensor.data, 6);

        uint16_t temp_value_raw = (Temp_Sensor.data[0] * 0x100U) + Temp_Sensor.data[1];
        uint8_t  temp_value_crc = Temp_Sensor.data[2];
        if (crc_check(&Temp_Sensor.data[0], 2, temp_value_crc) != 0U) {
            float_t            temperature = -45.0f + (175.0f * (float_t)temp_value_raw / (float_t)0xFFFF);
            FloatIntExtraction result      = convertInt(temperature);
            snprintf(buffer, sizeof(buffer), "Reading: %02d.%02d\r\n", result.Integer, result.Decimal);
            uart.LIB_UART_Write(buffer, sizeof(buffer));
        }
    }

    return 0;
}