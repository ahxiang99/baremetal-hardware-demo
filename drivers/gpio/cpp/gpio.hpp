#include "../low-level/gpio.h"
#include "../low-level/gpio_types.h"

using Result = status_t;

GPIO_InitTypeDef gpio_create_config(gpio_port_t, uint32_t, gpio_mode_t, gpio_otyper_t, gpio_ospeedr_t, gpio_pupdr_t, uint32_t);

class GPIO {
   private:
    GPIO_TypeDef*     m_pInstance;
    GPIO_InitTypeDef* m_pConfig;
    bool              m_Init;

   public:
    GPIO();
    GPIO(GPIO_InitTypeDef* _cfg);
    Result InitDriver(GPIO_InitTypeDef* p_Config);
    Result ResetDriver();

    Result SetPinConfig(GPIO_InitTypeDef* p_Config);  // Set Pin Configuration

    Result TogglePin(const uint16_t PIN);

    bool   IsInit();
};
