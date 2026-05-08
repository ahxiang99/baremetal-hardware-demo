#ifndef __MY_STM32F4_GPIO_LIB_H__
#define __MY_STM32F4_GPIO_LIB_H__

#include <sys/types.h>

#include <cstdint>

#include "Inc/drivers/my_stm32f4_gpio_driver.h"

typedef struct {
    uint32_t Pin;       /* Specifies the GPIO pins to be configured. */
    uint32_t Mode;      /* Specifies the operating mode for the selected pins. */
    uint32_t Pull;      /* Specifies the Pull-up or Pull-Down activation for the
                           selected pins. */
    uint32_t Speed;     /* Specifies the speed for the selected pins. */
    uint32_t Alternate; /* Peripheral to be connected to the selected pins. */
    uint32_t OutputType = 0;
} GPIO_InitTypeDef;

typedef uint32_t pinDataType;

typedef enum { GPIO_PIN_RESET = 0, GPIO_PIN_SET } GPIO_PinStateTypeDef;

typedef enum { GPIOA_PORT = 1, GPIOB_PORT, GPIOC_PORT, GPIOD_PORT, GPIOE_PORT, GPIOH_PORT } PORT_NameType;

typedef enum { GPIO_MODER, GPIO_PULL, GPIO_OSPEEDR, GPIO_ALTFN, GPIO_OTYPER } GPIO_RegisterType;

class Gpio {
   private:
    PORT_NameType    GPIO_port;
    GPIO_TypeDef     GPIO_regsStruct;
    GPIO_InitTypeDef GPIO_initStruct;

   public:
    Gpio(PORT_NameType port, GPIO_TypeDef* GPIOx, GPIO_InitTypeDef* initStruct);
    GPIO_PinStateTypeDef LIB_GPIO_ReadPin(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin);
    void                 LIB_GPIO_WriteOutputPin(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin, GPIO_PinStateTypeDef PinState);
    void                 LIB_GPIO_WritePin(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin, GPIO_PinStateTypeDef PinState);
    void                 LIB_GPIO_WriteRegister(GPIO_TypeDef* GPIOx, uint32_t Register, uint32_t GPIO_Pin, uint32_t PinState);
    void                 LIB_GPIO_TogglePin(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin);
    void                 LIB_RCC_GPIOA_CLK_ENABLE() const;
    void                 LIB_RCC_GPIOB_CLK_ENABLE() const;
    void                 LIB_RCC_GPIOC_CLK_ENABLE() const;
    void                 LIB_RCC_GPIOD_CLK_ENABLE() const;
    void                 LIB_RCC_GPIOE_CLK_ENABLE() const;
    void                 LIB_RCC_GPIOH_CLK_ENABLE() const;
};

GPIO_InitTypeDef __GPIO_PIN_PARAMS(pinDataType _Pin, pinDataType _Mode, pinDataType _Pull, pinDataType _Speed, pinDataType _Alternate);

#endif /* __MY_STM32F4_GPIO_LIB_H__ */