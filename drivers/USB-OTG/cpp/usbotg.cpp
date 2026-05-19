#include "usbotg.hpp"

#include "bit_utils.h"
#include "cpp/gpio.hpp"
#include "cpp/systick.hpp"
#include "logger.hpp"
#include "low-level/rcc.h"
#include "low-level/usbotg_registers.h"
#include "status.h"

extern MySysTick timer;

USB_OTG::USB_OTG() {}

status_t USB_OTG::init() {
    // 1. Enable 48MHz Clock
    if (Init48MHzForUSB() != STATUS_OK) return ERR_INIT;

    // 2. Enable USB_OTG Clock and PWR Interface
    uint32_t temp = RCC->AHB2ENR;
    SET_BIT(temp, RCC_AHB2ENR_OTGFS_EN);
    RCC->AHB2ENR = temp;
    LOG_INFO("USB-OTG-FS Clock Enabled.");

    temp = RCC->APB1ENR;
    SET_BIT(temp, RCC_APB1ENR_PWR_EN);
    RCC->APB1ENR = temp;
    LOG_INFO("Power Peripherals Clock Enabled.");

    // 3. Enable Peripherals Dedicated Alternate Function Routing on PA11 and PA12
    GPIO_Config USB_IO_Config{GPIO_PA, GPIO_PIN_11 | GPIO_PIN_12, GPIO_MODE_ALTFN, GPIO_OTYPER_PP, GPIO_OSPEEDR_VHS, GPIO_PUPDR_NOPULL, 10};
    GPIO        USB_IO;
    USB_IO.InitDriver(&USB_IO_Config);
    LOG_INFO("GPIOA PIN 11, 12 Configured Done.");

    // 4. Reset the USB Core
    temp = USB_OTG_FS->GREGS->OTG_FS_GRSTCTL;
    SET_BIT(temp, USB_OTG_FS_GRSTCTL_CSRST);
    USB_OTG_FS->GREGS->OTG_FS_GRSTCTL = temp;
    LOG_INFO("Addr: {}, USB Core Soft Reset Done.", Hex(USB_OTG_FS->GREGS->OTG_FS_GRSTCTL));

    // 5. Force Device Mode in the USB Configuration Register
    temp = USB_OTG_FS->GREGS->OTG_FS_GUSBCFG;
    SET_BIT(temp, USB_OTG_FS_GUSBCFG_FDMOD);
    USB_OTG_FS->GREGS->OTG_FS_GUSBCFG = temp;
    timer.delay_ms(25);
    LOG_INFO("Addr: {}, Force Device Mode Done.", Hex(USB_OTG_FS->GREGS->OTG_FS_GUSBCFG));

    // 6. Set Device Spped To Full Speed
    temp = USB_OTG_FS->DREGS->OTG_FS_DCFG;
    CLEAR_BIT(temp, 0x3);
    SET_BIT(temp, 0x3);
    USB_OTG_FS->DREGS->OTG_FS_DCFG = temp;
    LOG_INFO("Addr: {}, Set Device Speed Done.", Hex(USB_OTG_FS->DREGS->OTG_FS_DCFG));

    // 7. Power up the transceiver (Power and Clock Gating Register)
    CLEAR_BIT(OTG_FS_PCGCCTL, USB_OTG_FS_PCGCCTL_STPPCLK | USB_OTG_FS_PCGCCTL_GATEHCLK);
    LOG_INFO("Addr: {}, Power up the transceiver Done.", Hex(OTG_FS_PCGCCTL));

    // 8. Reconnect the physical transceiver lines (Disconnect bit off)
    CLEAR_BIT(USB_OTG_DEVICE->OTG_FS_DCTL, OTG_FS_DCTL_SDIS);
    LOG_INFO("Addr: {}, Reconnect the physical transceiver lines Done.", Hex(USB_OTG_FS->DREGS->OTG_FS_DCTL));

    // 9. Allocate RAM Space
    USB_OTG_FS->GREGS->OTG_FS_GRXFSIZ   = 128;

    USB_OTG_FS->GREGS->OTG_FS_HNPTXFSIZ = (128 << 16) | 64;

    CLEAR_BIT(USB_OTG_FS->DREGS->OTG_FS_DCTL, OTG_FS_DCTL_SDIS);

    return STATUS_OK;
}