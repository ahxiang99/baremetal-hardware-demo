#include "i2c.h"

#include <stdbool.h>
#include <string.h>

#include "bit_utils.h"
#include "i2c_types.h"
#include "low-level/i2c_bitfields.h"
#include "low-level/i2c_registers.h"
#include "low-level/i2c_types.h"
#include "low-level/nvic.h"
#include "low-level/rcc_bitfields.h"
#include "low-level/uart_bitfields.h"

static I2C_Handle_t hI2C1;

uint32_t            GetSysClockFreq() {
    /* Default using 16MHz */
    return 16000000;
}

uint32_t compute_CCR(I2C_Freq_t Freq) {
    return GetSysClockFreq() / (2 * Freq);
}

uint32_t compute_Trise() {
    return GetSysClockFreq() / 1000000 + 1;
}

I2C_TypeDef* I2C_GetBaseAddress(I2C_Num_t p_I2Cx) {
    switch (p_I2Cx) {
        case I2C_1:
            return I2C1;
        case I2C_2:
            return I2C2;
        case I2C_3:
            return I2C3;
        default:
            return 0x0U;
    }
}

I2C_Status I2C_HardwareInit(I2C_InitTypeDef* p_Config) {
    if (!p_Config) return I2C_ERR;

    // 1. Enable Clock for I2C
    switch (p_Config->i2cx) {
        case I2C_1:
            SET_BIT(RCC->APB1ENR, RCC_APB1ENR_I2C1_EN);
            break;
        case I2C_2:
            SET_BIT(RCC->APB1ENR, RCC_APB1ENR_I2C2_EN);
            break;
        case I2C_3:
            SET_BIT(RCC->APB1ENR, RCC_APB1ENR_I2C3_EN);
            break;
        default:
            return I2C_ERR;
    }

    I2C_TypeDef* p_Instance = I2C_GetBaseAddress(p_Config->i2cx);
    // 2. Disable I2C During Configuration
    CLEAR_BIT(p_Instance->CR1, I2C_CR1_PE);

    // 3.  Hardcode Peripheral Input Clock to 16 MHz.
    SET_BIT(p_Instance->CR2, I2C_CR2_16MHz);

    // 4. Configure Clock Control Register, Trise, Interrupt
    SET_BIT(p_Instance->CCR, compute_CCR(p_Config->I2C_FREQ));
    SET_BIT(p_Instance->TRISE, compute_Trise());

    My_NVIC_EnableIRQ(31);  // I2C1 Event Interrupt
    My_NVIC_EnableIRQ(32);  // I2C1 Error Interrupt

    // 5. Enable I2C Peripheral
    SET_BIT(p_Instance->CR1, I2C_CR1_PE);

    // 6. Also Init the Handle
    hI2C1.pInstance = p_Instance;
    hI2C1.pBuffer   = 0x00U;
    hI2C1.cmdBuffer = 0x00;
    hI2C1.Size      = 0;
    hI2C1.Count     = 0;
    hI2C1.State     = I2C_READY;

    return I2C_OK;
}

void I2C1_EV_IRQHandler(void) {
    // SB Flag: Start condition generated
    if (READ_BIT(hI2C1.pInstance->SR1, I2C_SR1_SB)) {
        hI2C1.pInstance->DR = hI2C1.cmdBuffer;
        return;
    }
    // ADDR Flag: Address sent and ACK received
    if (READ_BIT(hI2C1.pInstance->SR1, I2C_SR1_ADDR)) {
        if (hI2C1.State == I2C_BUSY_RX && hI2C1.Count == 1) {
            CLEAR_BIT(hI2C1.pInstance->CR1, I2C_CR1_ACK);
        }
        // Clear ADDR Bit
        uint32_t temp = hI2C1.pInstance->SR1;
        temp          = hI2C1.pInstance->SR2;
        (void)temp;

        if (hI2C1.State == I2C_BUSY_RX && hI2C1.Count == 1) {
            SET_BIT(hI2C1.pInstance->CR1, I2C_CR1_STOP);
        }
    }
    // TXE Flag: Data register empty, ready for next byte
    if (READ_BIT(hI2C1.pInstance->SR1, I2C_SR1_TXE) && !(READ_BIT(hI2C1.pInstance->SR1, I2C_SR1_BTF)) && (hI2C1.State == I2C_BUSY_TX)) {
        if (hI2C1.Count > 0) {
            hI2C1.pInstance->DR = *hI2C1.pBuffer++;
            hI2C1.Count--;
        }
    }
    // RNXE Flag: Data register is full, receive the byte
    if (READ_BIT(hI2C1.pInstance->SR1, I2C_SR1_RXNE)) {
        if (hI2C1.Count > 1) {
            if (hI2C1.Count == 2) {
                SET_BIT(hI2C1.pInstance->CR1, I2C_CR1_ACK);
                SET_BIT(hI2C1.pInstance->CR1, I2C_CR1_STOP);
            }
            *hI2C1.pBuffer++ = I2C1->DR;
            hI2C1.Count--;
        } else {
            *hI2C1.pBuffer++ = I2C1->DR;
            hI2C1.State      = I2C_READY;
            CLEAR_BIT(hI2C1.pInstance->CR2, I2C_CR2_ITEVTEN | I2C_CR2_ITBUFEN | I2C_CR2_ITERREN);
        }
    }
    // Wait for BTF (Byte Transfer Finished) to ensure last byte is gone
    if (READ_BIT(I2C1->SR1, I2C_SR1_BTF)) {
        if (hI2C1.State == I2C_BUSY_TX) {
            if (hI2C1.Count == 0) {
                // No more data, send STOP and wrap up
                SET_BIT(hI2C1.pInstance->CR1, I2C_CR1_STOP);
                CLEAR_BIT(hI2C1.pInstance->CR2, I2C_CR2_ITEVTEN | I2C_CR2_ITBUFEN | I2C_CR2_ITERREN);
                hI2C1.State = I2C_READY;
            }
        }
    }
}

void I2C1_ER_IRQHandler(void) {
    // 1. Acknowledge Failure (AF) - The Sensor didn't respond
    if (READ_BIT(I2C1->SR1, I2C_SR1_AF)) {
        // Clear flag: Write 0 to the bit
        CLEAR_BIT(I2C1->SR1, I2C_SR1_AF);

        // Action: Generate STOP to release the bus
        SET_BIT(I2C1->CR1, I2C_CR1_STOP);

        hI2C1.State = I2C_ERROR_NACK;
    }

    // 2. Bus Error (BERR) - Misplaced Start/Stop condition
    if (READ_BIT(I2C1->SR1, I2C_SR1_BERR)) {
        CLEAR_BIT(I2C1->SR1, I2C_SR1_BERR);
        hI2C1.State = I2C_ERROR_BUS;
    }

    // 3. Arbitration Lost (ARLO) - Another Master took the bus
    if (READ_BIT(I2C1->SR1, I2C_SR1_ARLO)) {
        CLEAR_BIT(I2C1->SR1, I2C_SR1_ARLO);
        hI2C1.State = I2C_ERROR_ARLO;
    }

    // 4. Overrun/Underrun (OVR) - CPU too slow for the clock speed
    if (READ_BIT(I2C1->SR1, I2C_SR1_OVR)) {
        CLEAR_BIT(I2C1->SR1, I2C_SR1_OVR);
        hI2C1.State = I2C_ERROR_OVR;
    }

    // CRITICAL: Disable interrupts so we don't loop forever in an error state
    CLEAR_BIT(I2C1->CR2, I2C_CR2_ITEVTEN | I2C_CR2_ITBUFEN | I2C_CR2_ITERREN);
}

I2C_Status I2C_Print(USART_TypeDef* p_Instance, const char* buffer, uint16_t max_size) {
    if (!p_Instance) return I2C_ERR;
    for (uint16_t i = 0; i < max_size; ++i) {
        USART_SendByte(p_Instance, buffer[i]);
    }
    return I2C_OK;
}

I2C_Status I2C_SendCommand(I2C_TypeDef* pInstance, uint8_t target_addr, uint8_t* pCmd, uint16_t pCmdSize) {
    if (!pInstance) return I2C_ERR;

    if (hI2C1.State != I2C_READY) return I2C_BUSY;

    hI2C1.cmdBuffer = target_addr;

    hI2C1.pBuffer   = pCmd;
    hI2C1.Size      = pCmdSize;
    hI2C1.Count     = pCmdSize;
    hI2C1.State     = I2C_BUSY_TX;

    // 1. Generate START Condition
    SET_BIT(pInstance->CR1, I2C_CR1_START);

    // 2. Enable Interrupt
    SET_BIT(pInstance->CR2, I2C_CR2_ITEVTEN | I2C_CR2_ITBUFEN | I2C_CR2_ITERREN);

    return I2C_OK;
}

I2C_Status I2C_ReceiveByte(I2C_TypeDef* pInstance, uint8_t target_addr, uint8_t* pData, uint16_t pDataSize) {
    if (!pInstance) return I2C_ERR;

    if (hI2C1.State != I2C_READY) return I2C_BUSY;
    hI2C1.cmdBuffer = target_addr | 0x01;
    hI2C1.pBuffer   = pData;
    hI2C1.Size      = pDataSize;
    hI2C1.Count     = pDataSize;
    hI2C1.State     = I2C_BUSY_RX;

    // 1. Generate START Condition
    SET_BIT(pInstance->CR1, I2C_CR1_START);
    SET_BIT(pInstance->CR1, I2C_CR1_ACK);

    // 2. Enable Interrupt
    SET_BIT(pInstance->CR2, I2C_CR2_ITEVTEN | I2C_CR2_ITBUFEN | I2C_CR2_ITERREN);

    return I2C_OK;
}

I2C_State I2C_GetState() {
    return hI2C1.State;
}

I2C_Status I2C_SetState(I2C_State pState) {
    hI2C1.State = pState;
    return I2C_OK;
}