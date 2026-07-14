#ifndef USBOTG_HPP
#define USBOTG_HPP

#include <cstdint>

#include "low-level/usbotg_bitfields.h"
#include "low-level/usbotg_registers.h"
#include "low-level/usbotg_types.h"
#include "status.h"

class USB_OTG {
   public:
    USB_OTG();
    status_t init();
};

#endif