#define XMODEM_SOH 0x01  // start of header
#define XMODEM_EOT 0x04  // end of transmission
#define XMODEM_ACK 0x06  // acknowledge
#define XMODEM_NAK 0x15  // not acknowledge
#define XMODEM_CAN 0x18  // cancel
#define XMODEM_C 0x43    // 'C' — request CRC mode

#include <cstdint>

#include "low-level/flash_bitfields.h"
#include "low-level/flash_registers.h"
#include "low-level/gpio_bitfields.h"
#include "low-level/gpio_registers.h"
#include "low-level/gpio_types.h"
#include "low-level/rcc_bitfields.h"
#include "low-level/rcc_registers.h"
#include "low-level/systick.h"
#include "low-level/uart_bitfields.h"
#include "low-level/uart_registers.h"
#include "low-level/uart_types.h"

void bootloader_gpio_init() {
    /* Configure Pin 13 Port C */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOC_EN;
    GPIOC->MODER &= ~(3U << 26);  // Set as Input Mode
    GPIOC->PUPDR &= ~(3U << 26);  // Clear Setting
    GPIOC->PUPDR |= (1U << 26);   // Clear Setting

    // Add PA5 LED:
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOA_EN;  // already enabled for UART
    GPIOA->MODER &= ~(3U << 10);           // PA5 clear
    GPIOA->MODER |= (1U << 10);            // PA5 output
}

void toggleLed() {
    GPIOA->ODR ^= (1U << 5);
}

void bootloader_uart_init() {
    /* Configure Uart 2 Pin 2 and 3 */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOA_EN;
    GPIOA->MODER &= ~(3U << 4);  // Clear
    GPIOA->MODER |= (2U << 4);   // Set ALTFN
    GPIOA->MODER &= ~(3U << 6);  // Clear
    GPIOA->MODER |= (2U << 6);   // Set ALTFN

    GPIOA->OSPEEDR &= ~(3U << 4);  // Clear
    GPIOA->OSPEEDR |= (3U << 4);   // Set VHS
    GPIOA->OSPEEDR &= ~(3U << 6);  // Clear
    GPIOA->OSPEEDR |= (3U << 6);   // Set VHS

    GPIOA->AFR[0] &= ~(0xF << 8);   // Clear
    GPIOA->AFR[0] |= (7U << 8);     // Set Uart2 AF
    GPIOA->AFR[0] &= ~(0xF << 12);  // Clear
    GPIOA->AFR[0] |= (7U << 12);    // Set Uart2 AF

    RCC->APB1ENR |= RCC_APB1ENR_USART2_EN;
    USART2->CR1 = 0;

    USART2->BRR = 0x8A;

    USART2->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;
}

void bootloader_uart_send(uint8_t byte) {
    while (!(USART2->SR & USART_SR_TXE));
    USART2->DR = byte;
}

uint8_t bootloader_uart_receive(uint32_t timeout_ms) {
    uint32_t count = timeout_ms * 16000;  // rough cycles per ms
    while (!(USART2->SR & USART_SR_RXNE)) {
        if (--count == 0) return 0xFF;  // timeout
    }
    return USART2->DR;
}

bool isUpdateRequested() {
    if (!(GPIOC->IDR & GPIO_PIN_13)) {
        return true;
    } else {
        return false;
    }
}

bool isAppValid() {
    uint32_t reset_handler = *reinterpret_cast<uint32_t*>(0x08004004);
    // Valid reset handler must be in application flash region:
    return (reset_handler >= 0x08004000 && reset_handler < 0x08080000);
}

void jumpToApplication() {
    // Step 1 — disable IRQ:
    __asm volatile("CPSID I");

    // Step 2 — disable SysTick:
    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL  = 0;

    // Step 3 — set VTOR:
    *reinterpret_cast<volatile uint32_t*>(0xE000ED08) = 0x08004000;

    // Step 4 — set MSP:
    uint32_t initial_sp = *reinterpret_cast<uint32_t*>(0x08004000);
    __asm volatile("MSR msp, %0" ::"r"(initial_sp));

    __asm volatile("CPSIE I");  // ← ADD THIS

    // Step 5 — jump:
    void (*app_reset)(void) = reinterpret_cast<void (*)()>(*reinterpret_cast<uint32_t*>(0x08004004));
    app_reset();  // ← if we reach here, 4 blinks seen
}

uint16_t crc16_calculate(const uint8_t* data, uint32_t length) {
    uint16_t crc = 0x0000;  // ← XMODEM starts at 0
    for (uint32_t i = 0; i < length; i++) {
        crc ^= (static_cast<uint16_t>(data[i]) << 8);
        for (uint8_t bit = 0; bit < 8; bit++) {
            crc = (crc & 0x8000) ? (crc << 1) ^ 0x1021 : (crc << 1);
        }
    }
    return crc;
}

bool flashWrite(uint32_t address, const uint8_t* data, uint32_t length) {
    // Write word by word (4 bytes at a time):
    for (uint32_t i = 0; i < length; i += 4) {
        *reinterpret_cast<volatile uint32_t*>(address + i) = *reinterpret_cast<const uint32_t*>(data + i);
        while (FLASH->SR & FLASH_SR_BSY);
    }

    return !(FLASH->SR & FLASH_SR_WRPERR);  // return false if write protected
}

void eraseApplicationSectors() {
    // Erase sectors 1-7 (application area):
    for (uint8_t sector = 1; sector <= 4; sector++) {
        // Set sector erase mode:
        FLASH->CR = 0;
        FLASH->CR = FLASH_CR_SER | (sector << 3) | FLASH_CR_STRT;  // FLASH_CR_SER + sector number + FLASH_CR_STRT
        // Wait for completion:
        while (FLASH->SR & FLASH_SR_BSY);  // FLASH_SR_BSY
    }
    FLASH->CR = 0;
}

uint8_t checksum_calculate(const uint8_t* data, uint32_t length) {
    uint8_t sum = 0;
    for (uint32_t i = 0; i < length; i++) sum += data[i];
    return sum;
}

void FirmwareUpdate() {
    // Phase 1 — erase:
    FLASH->KEYR = 0x45670123;
    FLASH->KEYR = 0xCDEF89AB;

    // Phase 2 — unlock flash for writing:
    eraseApplicationSectors();

    FLASH->CR |= FLASH_CR_PG;
    FLASH->CR |= (2 << 8);
    // Phase 3 — handshake: send 'C' every second until SOH:
    uint8_t first = 0;
    while (first != XMODEM_SOH) {
        bootloader_uart_send(XMODEM_C);
        first = bootloader_uart_receive(1000);
    }

    // Phase 4 — receive packets:
    uint32_t write_address  = 0x08004000;
    uint8_t  expected_block = 1;
    uint8_t  packet[133];

    do {
        if (first == XMODEM_SOH) {
            for (int i = 0; i < 132; i++) packet[i] = bootloader_uart_receive(100);

            uint8_t  block_num  = packet[0];
            uint8_t  block_comp = packet[1];
            uint8_t* data       = &packet[2];
            uint16_t crc_recv   = static_cast<uint16_t>((packet[130] << 8) | packet[131]);
            uint16_t crc_calc   = crc16_calculate(data, 128);

            bool     valid      = (block_num == expected_block) && (block_comp == (uint8_t)~expected_block) && (crc_calc == crc_recv);

            if (valid) {
                flashWrite(write_address, data, 128);
                write_address += 128;
                expected_block++;
                bootloader_uart_send(XMODEM_ACK);
            } else {
                bootloader_uart_send(XMODEM_NAK);
            }
        }
        first = bootloader_uart_receive(30000);

    } while (first != XMODEM_EOT);

    // Phase 5 — EOT:
    bootloader_uart_send(XMODEM_ACK);
    FLASH->CR &= ~FLASH_CR_PG;
    FLASH->CR |= FLASH_CR_LOCK;

    // Blink 5 times before jumping:
    for (int i = 0; i < 5; i++) {
        toggleLed();
        for (volatile int d = 0; d < 500000; d++);
        toggleLed();
        for (volatile int d = 0; d < 500000; d++);
    }

    jumpToApplication();
}

void FirmwareUpdate2() {
    // Unlock:
    FLASH->KEYR = 0x45670123;
    FLASH->KEYR = 0xCDEF89AB;

    // Erase:
    eraseApplicationSectors();

    // Enable programming:
    FLASH->CR |= FLASH_CR_PG;
    FLASH->CR |= (2 << 8);

    // Test write — write known pattern to 0x08004000:
    uint8_t test[4] = {0xAA, 0xBB, 0xCC, 0xDD};
    flashWrite(0x08004000, test, 4);

    // Lock:
    FLASH->CR &= ~FLASH_CR_PG;
    FLASH->CR |= FLASH_CR_LOCK;

    // Blink to signal done:
    for (int i = 0; i < 3; i++) {
        toggleLed();
        for (volatile int d = 0; d < 500000; d++);
        toggleLed();
        for (volatile int d = 0; d < 500000; d++);
    }

    while (1);  // halt — don't jump
}

int main() {
    bootloader_gpio_init();
    bootloader_uart_init();

    // Blink pattern — show decision:
    if (isAppValid()) {
        // 3 slow blinks — app valid, will jump:
        for (int i = 0; i < 3; i++) {
            toggleLed();
            for (volatile int d = 0; d < 1000000; d++);
            toggleLed();
            for (volatile int d = 0; d < 1000000; d++);
        }
    } else {
        // 10 fast blinks — app invalid:
        for (int i = 0; i < 10; i++) {
            toggleLed();
            for (volatile int d = 0; d < 200000; d++);
            toggleLed();
            for (volatile int d = 0; d < 200000; d++);
        }
    }

    if (isUpdateRequested() || !isAppValid()) {
        FirmwareUpdate();
    } else {
        jumpToApplication();
    }
}