#ifndef __MY_STM32F4_GPIO_LIB_H__
#define __MY_STM32F4_GPIO_LIB_H__

#include <sys/types.h>

#include <cstdint>

#include "Inc/drivers/my_stm32f4_gpio_driver.h"

#define GPIO_PIN_O ((uint16_t)0x0001)   /* Pin 0 selected    */
#define GPIO_PIN_1 ((uint16_t)0x0002)   /* Pin 1 selected    */
#define GPIO_PIN_2 ((uint16_t)0x0004)   /* Pin 2 selected    */
#define GPIO_PIN_3 ((uint16_t)0x0008)   /* Pin 3 selected    */
#define GPIO_PIN_4 ((uint16_t)0x0010)   /* Pin 4 selected    */
#define GPIO_PIN_5 ((uint16_t)0x0020)   /* Pin 5 selected    */
#define GPIO_PIN_6 ((uint16_t)0x0040)   /* Pin 6 selected    */
#define GPIO_PIN_7 ((uint16_t)0x0080)   /* Pin 7 selected    */
#define GPIO_PIN_8 ((uint16_t)0x0100)   /* Pin 8 selected    */
#define GPIO_PIN_9 ((uint16_t)0x0200)   /* Pin 9 selected    */
#define GPIO_PIN_10 ((uint16_t)0x0400)  /* Pin 10 selected   */
#define GPIO_PIN_11 ((uint16_t)0x0800)  /* Pin 11 selected   */
#define GPIO_PIN_12 ((uint16_t)0x1000)  /* Pin 12 selected   */
#define GPIO_PIN_13 ((uint16_t)0x2000)  /* Pin 13 selected   */
#define GPIO_PIN_14 ((uint16_t)0x4000)  /* Pin 14 selected   */
#define GPIO_PIN_15 ((uint16_t)0x8000)  /* Pin 15 selected   */
#define GPIO_PIN_ALL ((uint16_t)0xFFFF) /* All pins selected */

#define GPIO_MODE_INPUT 0x00U
#define GPIO_MODE_OUTPUT_PP 0x01U
#define GPIO_MODE_OUTPUT_OD 0x11U
#define GPIO_MODE_ALTFN_PP 0x02U
#define GPIO_MODE_ALTFN_OD 0x12U

#define GPIO_MODE_ANALOG 0x03U

#define GPIO_NOPULL 0x00U
#define GPIO_PULLUP 0x01U
#define GPIO_PULLDOWN 0x02U

typedef struct {
    uint32_t Pin;       /* Specifies the GPIO pins to be configured. */
    uint32_t Mode;      /* Specifies the operating mode for the selected pins. */
    uint32_t Pull;      /* Specifies the Pull-up or Pull-Down activation for the
                           selected pins. */
    uint32_t Speed;     /* Specifies the speed for the selected pins. */
    uint32_t Alternate; /* Peripheral to be connected to the selected pins. */
} GPIO_InitTypeDef;

typedef uint32_t pinDataType;

typedef enum { GPIO_PIN_RESET = 0, GPIO_PIN_SET } GPIO_PinStateTypeDef;

typedef enum { GPIOA_PORT = 1, GPIOB_PORT, GPIOC_PORT, GPIOD_PORT, GPIOE_PORT, GPIOH_PORT } PORT_NameType;

class Gpio {
   private:
    PORT_NameType    GPIO_port;
    GPIO_TypeDef     GPIO_regsStruct;
    GPIO_InitTypeDef GPIO_initStruct;

   public:
    Gpio(PORT_NameType port, GPIO_TypeDef* GPIOx, GPIO_InitTypeDef* initStruct);
    GPIO_PinStateTypeDef LIB_GPIO_ReadPin(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin);
    void                 LIB_GPIO_WritePin(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin, GPIO_PinStateTypeDef PinState);
    void                 LIB_GPIO_TogglePin(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin);
    void                 __LIB_RCC_GPIOA_CLK_ENABLE() const;
    void                 __LIB_RCC_GPIOB_CLK_ENABLE() const;
    void                 __LIB_RCC_GPIOC_CLK_ENABLE() const;
    void                 __LIB_RCC_GPIOD_CLK_ENABLE() const;
    void                 __LIB_RCC_GPIOE_CLK_ENABLE() const;
    void                 __LIB_RCC_GPIOH_CLK_ENABLE() const;
};

GPIO_InitTypeDef __GPIO_PIN_PARAMS(pinDataType _Pin, pinDataType _Mode, pinDataType _Pull, pinDataType _Speed, pinDataType _Alternate);

#endif /* __MY_STM32F4_GPIO_LIB_H__ */