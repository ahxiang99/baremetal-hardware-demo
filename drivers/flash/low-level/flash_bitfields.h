#pragma once

#define FLASH_ACR_LATENCY_Pos 0
#define FLASH_ACR_LATENCY_Msk (0xFU << FLASH_ACR_LATENCY_Pos)

#define FLASH_CR_PG (1 << 0)     // programming
#define FLASH_CR_SER (1 << 1)    // sector erase
#define FLASH_CR_SNB (3 << 3)    // sector number bits [6:3]
#define FLASH_CR_STRT (1 << 16)  // start erase
#define FLASH_CR_LOCK (1 << 31)  // lock
#define FLASH_SR_BSY (1 << 16)   // busy
#define FLASH_SR_WRPERR (1 << 4)