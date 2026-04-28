#include "Inc/libs/my_stm32f4_gpio_lib.h"
#include "Inc/libs/my_stm32f4_systick_lib.h"
#include "Inc/libs/my_stm32f4_uart_lib.h"

int main() {
    GPIO_InitTypeDef my_ledInitStruct = {GPIO_PIN_5, GPIO_MODE_OUTPUT_PP, GPIO_NOPULL, 0, 0};
    Gpio             led(GPIOA_PORT, GPIOA, &my_ledInitStruct);

    while (1) {
        led.LIB_GPIO_TogglePin(GPIOA, GPIO_PIN_5);  // Toggle the LED
        MySysTick(1);                               // Wait for 1 second
    }

    return 0;
}