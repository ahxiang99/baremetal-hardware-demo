#ifndef GPIO_TYPES_H
#define GPIO_TYPES_H

/* PORT - PORT A to H */
typedef enum { GPIO_PA, GPIO_PB, GPIO_PC, GPIO_PD, GPIO_PE, GPIO_PH } gpio_port_t;

/* MODER -  IO PIN Operating Mode */
typedef enum { GPIO_MODE_INPUT = 0, GPIO_MODE_OUTPUT, GPIO_MODE_ALTFN, GPIO_MODE_ANALOG } gpio_mode_t;

/* OTYPER - Output Type */
typedef enum { GPIO_OTYPER_PP = 0, GPIO_OTYPER_OD } gpio_otyper_t;

/* OSPEED - Output Speed */
typedef enum { GPIO_OSPEEDR_LS = 0, GPIO_OSPEEDR_MS, GPIO_OSPEEDR_HS, GPIO_OSPEEDR_VHS } gpio_ospeedr_t;

/* PUPDR - Pull Up Pull Down Register */
typedef enum { GPIO_PUPDR_NOPULL = 0, GPIO_PUPDR_PULLUP, GPIO_PUPDR_PULLDOWN } gpio_pupdr_t;

/* Pin State */
typedef enum { GPIO_PIN_RESET = 0, GPIO_PIN_SET } gpio_pin_state_t;

/* Error State */
typedef enum { GPIO_OK = 1, GPIO_ERR = -1 } gpio_status_t;

typedef struct {
    gpio_port_t    PORT;      /* Specifies the GPIO PORT to be configured. */
    uint32_t       PIN;       /* Specifies the GPIO pins to be configured. */
    gpio_mode_t    MODE;      /* Specifies the operating mode for the selected pins. */
    gpio_otyper_t  OTYPE;     /* Specifies the Output Type for the selected pins. */
    gpio_ospeedr_t SPD;       /* Specifies the speed for the selected pins. */
    gpio_pupdr_t   PUPD;      /* Specifies the Pull-up or Pull-Down activation for the selected pins. */
    uint32_t       Alternate; /* Peripheral to be connected to the selected pins. */
} GPIO_InitTypeDef;

#endif