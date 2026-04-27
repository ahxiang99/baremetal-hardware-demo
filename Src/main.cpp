#include "Inc/libs/my_stm32f4_gpio_lib.h"

int main() {
    GPIO_InitTypeDef GPIO_InitStruct = __GPIO_PIN_PARAMS(GPIO_PIN_5, GPIO_MODE_OUTPUT_PP, GPIO_NOPULL, 0x02U, 0x00U);

    /* Test 1 */
    Gpio myOutput_1(GPIOA_PORT, GPIOA, &GPIO_InitStruct);

    Gpio myOutput_2(GPIOC_PORT, GPIOC, &GPIO_InitStruct);

    while (1) {
        myOutput_1.LIB_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
        myOutput_2.LIB_GPIO_TogglePin(GPIOC, GPIO_PIN_5);
        for (volatile int i = 0; i < 100000; i++); /* Simple delay */
    }

    return 0;
}