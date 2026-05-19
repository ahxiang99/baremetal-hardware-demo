#ifndef USBOTG_REGISTERS_H
#define USBOTG_REGISTERS_H

#include <stdint.h>

typedef struct {
    volatile uint32_t OTG_FS_GOTGCTL;   /*!< OTG_FS control and status register,          Address offset: 0x00 */
    volatile uint32_t OTG_FS_GOTGINT;   /*!< OTG_FS interrupt register,                   Address offset: 0x04 */
    volatile uint32_t OTG_FS_GAHBCFG;   /*!< OTG_FS AHB configuration register,           Address offset: 0x08 */
    volatile uint32_t OTG_FS_GUSBCFG;   /*!< OTG_FS USB configuration register,           Address offset: 0x0C */
    volatile uint32_t OTG_FS_GRSTCTL;   /*!< OTG_FS reset register,                       Address offset: 0x10 */
    volatile uint32_t OTG_FS_GINTSTS;   /*!< OTG_FS core interrupt register,              Address offset: 0x14 */
    volatile uint32_t OTG_FS_GINTMSK;   /*!< OTG_FS interrupt mask register,               Address offset: 0x18 */
    volatile uint32_t OTG_FS_GRXSTSR;   /*!< OTG_FS Receive status debug read register,   Address offset: 0x1C */
    volatile uint32_t OTG_FS_GRXSTSP;   /*!< OTG_FS Receive status debug read/pop register, Address offset: 0x20 */
    volatile uint32_t OTG_FS_GRXFSIZ;   /*!< OTG_FS Receive FIFO size register,           Address offset: 0x24 */
    volatile uint32_t OTG_FS_HNPTXFSIZ; /*!< OTG_FS Host non-periodic transmit FIFO size, Address offset: 0x28 */
    volatile uint32_t OTG_FS_HNPTXSTS;  /*!< OTG_FS Non-periodic transmit FIFO status,    Address offset: 0x2C */
    uint32_t          Reserved_1[2];    /*!< Reserved,                                    Address offset: 0x30-0x34 */
    volatile uint32_t OTG_FS_GCCFG;     /*!< OTG_FS general core configuration register,  Address offset: 0x38 */
    volatile uint32_t OTG_FS_CID;       /*!< OTG_FS core ID register,                     Address offset: 0x3C */
    uint32_t          Reserved_2[48];   /*!< Reserved,                                    Address offset: 0x40-0xFC */
    volatile uint32_t OTG_FS_HPTXFSIZ;  /*!< OTG_FS Host periodic transmit FIFO size,     Address offset: 0x100 */
    volatile uint32_t OTG_FS_DIEPTXFx;  /*!< OTG_FS device IN endpoint transmit FIFO x,   Address offset: 0x104 */
} USB_OTG_GREGS;                        // Global Registers

typedef struct {
    volatile uint32_t OTG_FS_HCFG;     /*!< OTG_FS host configuration register,          Address offset: 0x400 */
    volatile uint32_t OTG_FS_HFIR;     /*!< OTG_FS host frame interval register,          Address offset: 0x404 */
    volatile uint32_t OTG_FS_HFNUM;    /*!< OTG_FS host frame number/frame time remain,  Address offset: 0x408 */
    uint32_t          Reserved_3;      /*!< Reserved,                                    Address offset: 0x40C */
    volatile uint32_t OTG_FS_HPTXSTS;  /*!< OTG_FS host periodic transmit FIFO status,   Address offset: 0x410 */
    volatile uint32_t OTG_FS_HAINT;    /*!< OTG_FS host all endpoints interrupt register, Address offset: 0x414 */
    volatile uint32_t OTG_FS_HAINTMSK; /*!< OTG_FS host all endpoints interrupt mask,    Address offset: 0x418 */
    volatile uint32_t OTG_FS_HPRT;     /*!< OTG_FS host port control and status register, Address offset: 0x440 */
} USB_OTG_HREGS;                       // Host Mode Registers

typedef struct {
    volatile uint32_t OTG_FS_HCCHARx;   /*!< OTG_FS host channel characteristics register, Address offset: 0x00 */
    uint32_t          Reserved_4;       /*!< Reserved,                                    Address offset: 0x04 */
    volatile uint32_t OTG_FS_HCINTx;    /*!< OTG_FS host channel interrupt register,       Address offset: 0x08 */
    volatile uint32_t OTG_FS_HCINTMSKx; /*!< OTG_FS host channel interrupt mask register,  Address offset: 0x0C */
    volatile uint32_t OTG_FS_HCTSIZx;   /*!< OTG_FS host channel transfer size register,   Address offset: 0x10 */
    uint32_t          Reserved_5[3];    /*!< Reserved,                                    Address offset: 0x14-0x1C */
} USB_OTG_HC_REGS;                      // Host Channel

typedef struct {
    volatile uint32_t OTG_FS_DCFG;       /*!< OTG_FS device configuration register,         Address offset: 0x800 */
    volatile uint32_t OTG_FS_DCTL;       /*!< OTG_FS device control register,               Address offset: 0x804 */
    volatile uint32_t OTG_FS_DSTS;       /*!< OTG_FS device status register,                Address offset: 0x808 */
    uint32_t          Reserved_Gap;      /*!< Reserved,                                    Address offset: 0x80C */
    volatile uint32_t OTG_FS_DIEPMSK;    /*!< OTG_FS device IN endpoint interrupt mask reg, Address offset: 0x810 */
    volatile uint32_t OTG_FS_DOEPMSK;    /*!< OTG_FS device OUT endpoint interrupt mask reg, Address offset: 0x814 */
    volatile uint32_t OTG_FS_DAINT;      /*!< OTG_FS device all endpoints interrupt reg,    Address offset: 0x818 */
    volatile uint32_t OTG_FS_DAINTMSK;   /*!< OTG_FS device all endpoints interrupt mask,   Address offset: 0x81C */
    uint32_t          Reserved_6[2];     /*!< Reserved,                                    Address offset: 0x820-0x824 */
    volatile uint32_t OTG_FS_DVBUSDIS;   /*!< OTG_FS device VBUS discharge time register,   Address offset: 0x828 */
    volatile uint32_t OTG_FS_DVBUSPULSE; /*!< OTG_FS device VBUS pulsing time register,    Address offset: 0x82C */
    uint32_t          Reserved_7;        /*!< Reserved,                                    Address offset: 0x830 */
    volatile uint32_t OTG_FS_DIEPEMPMSK; /*!< OTG_FS device IN endpoint FIFO empty mask,    Address offset: 0x834 */
} USB_OTG_DREGS;                         // Device mode registers

typedef struct {
    volatile uint32_t OTG_FS_DIEPCTLx;  /*!< OTG_FS device IN endpoint control register,   Address offset: 0x00 */
    uint32_t          Reserved_7;       /*!< Reserved,                                    Address offset: 0x04 */
    volatile uint32_t OTG_FS_DIEPINTx;  /*!< OTG_FS device IN endpoint interrupt register, Address offset: 0x08 */
    uint32_t          Reserved_8;       /*!< Reserved,                                    Address offset: 0x0C */
    volatile uint32_t OTG_FS_DIEPTSIZx; /*!< OTG_FS device IN endpoint transfer size reg,  Address offset: 0x10 */
    uint32_t          Reserved_9;       /*!< Reserved,                                    Address offset: 0x14 */
    volatile uint32_t OTG_FS_DTXFSTS;   /*!< OTG_FS device IN endpoint transmit status,    Address offset: 0x18 */
    uint32_t          Reserved_10;      /*!< Reserved,                                    Address offset: 0x1C */
} USB_OTG_INEPREGS;                     // In Endpoint Control Registers

typedef struct {
    volatile uint32_t OTG_FS_DOEPCTLx;  /*!< OTG_FS device OUT endpoint control register,  Address offset: 0x00 */
    uint32_t          Reserved_7;       /*!< Reserved,                                    Address offset: 0x04 */
    volatile uint32_t OTG_FS_DOEPINTx;  /*!< OTG_FS device OUT endpoint interrupt register, Address offset: 0x08 */
    uint32_t          Reserved_8;       /*!< Reserved,                                    Address offset: 0x0C */
    volatile uint32_t OTG_FS_DOEPTSIZx; /*!< OTG_FS device OUT endpoint transfer size reg, Address offset: 0x10 */
    uint32_t          Reserved_9[3];    /*!< Reserved,                                    Address offset: 0x14-0x1C */
} USB_OTG_OUTEPREGS;                    // Out Endpoint Control Registers

#define USB_MAX_ENDPOINT 4
#define USB_MAX_HOST 8
#define USB_OTG_FS_BASE (0x50000000U)

#define OTG_FS_PCGCCTL (*((volatile uint32_t*)(USB_OTG_FS_BASE + 0xE00)))

#define USB_OTG_GREGS_BASE ((USB_OTG_GREGS*)(USB_OTG_FS_BASE))
#define USB_OTG_HREGS_BASE ((USB_OTG_HREGS*)(USB_OTG_FS_BASE + 0x400))
#define USB_OTG_HC_REGS_BASE ((USB_OTG_HC_REGS*)(USB_OTG_FS_BASE + 0x500))
#define USB_OTG_DEVICE ((USB_OTG_DREGS*)(USB_OTG_FS_BASE + 0x800))
#define USB_OTG_INEPREGS_BASE ((USB_OTG_INEPREGS*)(USB_OTG_FS_BASE + 0x900))
#define USB_OTG_OUTEPREGS_BASE ((USB_OTG_OUTEPREGS*)(USB_OTG_FS_BASE + 0xB00))

struct USB_OTG_CORE_TypeDef {
    USB_OTG_GREGS* const     GREGS = USB_OTG_GREGS_BASE;
    USB_OTG_HREGS* const     HREGS = USB_OTG_HREGS_BASE;
    USB_OTG_DREGS* const     DREGS = USB_OTG_DEVICE;
    USB_OTG_HC_REGS* const   HC_REGS[USB_MAX_HOST];
    USB_OTG_INEPREGS* const  INEP[USB_MAX_ENDPOINT];
    USB_OTG_OUTEPREGS* const OUTEP[USB_MAX_ENDPOINT];

    USB_OTG_CORE_TypeDef()
        : HC_REGS{&((USB_OTG_HC_REGS*)(USB_OTG_FS_BASE + 0x500))[0], &((USB_OTG_HC_REGS*)(USB_OTG_FS_BASE + 0x500))[1], &((USB_OTG_HC_REGS*)(USB_OTG_FS_BASE + 0x500))[2],
                  &((USB_OTG_HC_REGS*)(USB_OTG_FS_BASE + 0x500))[3], &((USB_OTG_HC_REGS*)(USB_OTG_FS_BASE + 0x500))[4], &((USB_OTG_HC_REGS*)(USB_OTG_FS_BASE + 0x500))[5],
                  &((USB_OTG_HC_REGS*)(USB_OTG_FS_BASE + 0x500))[6], &((USB_OTG_HC_REGS*)(USB_OTG_FS_BASE + 0x500))[7]},
          INEP{&USB_OTG_INEPREGS_BASE[0], &USB_OTG_INEPREGS_BASE[1], &USB_OTG_INEPREGS_BASE[2], &USB_OTG_INEPREGS_BASE[3]},
          OUTEP{&USB_OTG_OUTEPREGS_BASE[0], &USB_OTG_OUTEPREGS_BASE[1], &USB_OTG_OUTEPREGS_BASE[2], &USB_OTG_OUTEPREGS_BASE[3]} {}
};

inline const USB_OTG_CORE_TypeDef USB_OTG_FS_Mapping{};
#define USB_OTG_FS (&USB_OTG_FS_Mapping)

#endif  // USBOTG_REGISTERS_H