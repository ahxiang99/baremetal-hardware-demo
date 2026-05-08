#include "Inc/libs/my_stm32f4_i2c_lib.h"

#include "Inc/libs/my_stm32f4_gpio_lib.h"
#include "my_stm32f4_systick_lib.h"

uint32_t I2C::GetSysClockFreq() {
    /* Default using 16MHz */
    return 16000000;
}

uint32_t I2C::GetCCR() {
    return GetSysClockFreq() / (2 * I2C_initStruct->SCL_ClkFreq);
}

I2C::I2C(I2C_TypeDef* _i2c, I2C_InitTypeDef* _init) : I2Cx(_i2c), I2C_initStruct(_init) {
    // Enable Port B and RCC CLOCK
    RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;
    RCC->AHB1ENR |= GPIOBEN;
    GPIO_InitTypeDef I2C_GPIO_initStruct{GPIO_PIN_8 | GPIO_PIN_9, GPIO_MODE_ALTFN, GPIO_PULLUP, 0, 0x4, 1};
    Gpio             GPIO_I2C{GPIOB_PORT, GPIOB, &I2C_GPIO_initStruct};

    /* During I2C Peripherals Configuration, Enable bit must reset.*/
    I2Cx->CR1 &= ~(1 << 0);

    /* Program the peripheral input clock in I2C_CR2 register in order to generate correct timings */
    I2Cx->CR2 = (1 << 4); /* Set Peripheral input clock to 16 MHz*/

    /* Configure the clock control registers */
    I2Cx->CCR = (0 << 15);
    I2Cx->CCR |= (I2C_DUTYCYCLE_2 << 14);
    I2Cx->CCR |= GetCCR();

    /* Configure the rise time register */
    I2Cx->TRISE = 17;

    I2Cx->CR1 |= I2C_ENABLE;
}

int32_t I2C::LIB_I2C_TRANSMIT(uint32_t dev_addr, uint8_t* cmd, uint16_t size) {
    /* 1. Wait for bus is not busy */
    while (I2Cx->SR2 & I2C_SR2_BUSY);

    /* 2. Generate the START condition */
    I2Cx->CR1 |= I2C_CR1_START;

    // 3. Wait for the SB (Start Bit) flag to be set
    while (!(I2Cx->SR1 & I2C_SR1_SB));

    // 4. Send the Device Address (Left shifted) with the Write bit (0)
    // DeviceAddr should be the 7-bit address (e.g., 0x50 << 1)
    I2Cx->DR = (dev_addr << 1);
    // 5. Wait for the ADDR flag (Address matched)
    while (!(I2Cx->SR1 & I2C_SR1_ADDR)) {
        if (I2Cx->SR2 & I2C_SR2_BUSY) {
            continue;
        }
    }

    // 6. Clear ADDR flag by reading SR1 followed by SR2
    uint32_t temp = I2Cx->SR1;
    temp          = I2Cx->SR2;
    (void)temp;  // Prevent "unused variable" warning

    // 7. Transmit the data bytes
    for (uint16_t i = 0; i < size; i++) {
        // Wait until TXE (Transmit buffer empty) is set
        while (!(I2Cx->SR1 & I2C_SR1_TXE));

        // Write data to the Data Register
        I2Cx->DR = cmd[i];
    }

    // 8. Wait for BTF (Byte Transfer Finished) to ensure last byte is gone
    while (!(I2Cx->SR1 & I2C_SR1_BTF));

    // 9. Generate the STOP condition
    I2Cx->CR1 |= I2C_CR1_STOP;

    return I2C_OK;
}

int32_t I2C::LIB_I2C_READ_REGISTER(uint32_t dev_addr, uint8_t* pData, uint16_t size) {
    /* 1. Wait for bus & Generate START to send instruction to get data */
    while (I2Cx->SR2 & I2C_SR2_BUSY);

    // 2. Enable Acknowledgement (ACK) so the sensor knows to keep sending
    I2Cx->CR1 |= I2C_CR1_ACK;

    // 3. Generate the START condition (Bit 8 of CR1)
    I2Cx->CR1 |= I2C_CR1_START;
    while (!(I2Cx->SR1 & I2C_SR1_SB));  // Wait for Start Bit flag

    // 5. Send Device Address + Read (1)
    I2Cx->DR = (dev_addr << 1) | 0x01;
    while (!(I2Cx->SR1 & I2C_SR1_ADDR));  // Wait for Match Address

    // Clear ADDR flag: Read SR1 then SR2
    uint32_t temp = I2Cx->SR1;
    temp          = I2Cx->SR2;
    (void)temp;

    // 5. Handle different payload sizes
    if (size == 1) {
        // Special case for single byte read
        I2Cx->CR1 &= ~I2C_CR1_ACK;  // Disable ACK immediately

        I2Cx->CR1 |= I2C_CR1_STOP;  // Program the STOP condition

        while (!(I2Cx->SR1 & I2C_SR1_RXNE));  // Wait for Data Register Not Empty
        pData[0] = I2Cx->DR;                  // Read the byte

    } else {
        // Multi-byte read (e.g., 6 bytes for SHT40)
        uint32_t temp = I2Cx->SR1;  // Clear ADDR flag
        temp          = I2Cx->SR2;
        (void)temp;

        for (uint16_t i = 0; i < size; i++) {
            // Before receiving the last byte, we must NACK and STOP
            if (i == (size - 1)) {
                I2Cx->CR1 &= ~I2C_CR1_ACK;  // Send NACK to end communication
                I2Cx->CR1 |= I2C_CR1_STOP;  // Program the STOP condition
            }

            // Wait for data to arrive in the Shift Register
            while (!(I2Cx->SR1 & I2C_SR1_RXNE));

            // Read data from DR (this also clears RXNE)
            pData[i] = I2Cx->DR;
        }
    }
    return I2C_OK;
}

/* • Set the START bit in the I2C_CR1 register to generate a Start condition */

/* Following the address transmission and after clearing ADDR, the I2C interface enters
controller receiver mode. In this mode the interface receives bytes from the SDA line into
the DR register via the internal shift register. After each byte the interface generates in
sequence:
1. An acknowledge pulse if the ACK bit is set
2. The RxNE bit is set and an interrupt is generated if the ITEVFEN and ITBUFEN bits are
set (see Figure 165 Transfer sequencing EV7).
If the RxNE bit is set and the data in the DR register is not read before the end of the last
data reception, the BTF bit is set by hardware and the interface waits until BTF is cleared by
a read in the DR register, stretching SCL low.

Closing the communication
The controller sends a NACK for the last byte received from the target. After receiving this
NACK, the target releases the control of the SCL and SDA lines. Then the controller can
send a Stop/Restart condition.
1. To generate the nonacknowledge pulse after the last received data byte, the ACK bit
must be cleared just after reading the second last data byte (after second last RxNE
event).
2. In order to generate the Stop/Restart condition, software must set the STOP/START bit
after reading the second last data byte (after the second last RxNE event).
3. In case a single byte has to be received, the Acknowledge disable is made during EV6
(before ADDR flag is cleared) and the STOP condition generation is made after EV6.
After the Stop condition generation, the interface goes automatically back to target mode
(MSL bit cleared). */