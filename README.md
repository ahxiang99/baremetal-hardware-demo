# STM32F401XE GPIO & Peripheral Driver

A C++17 embedded firmware project for STM32F401XE microcontroller with layered drivers for GPIO, UART, I2C, and SysTick interfaces.

## Target Hardware

- **MCU**: STM32F401XE (ARM Cortex-M4, 84 MHz)
- **Flash**: 512 KB | **RAM**: 96 KB
- **Features**: Hardware FPU, SysTick Timer

## Project Structure

```
baremetal-hardware-demo/
├── CMakeLists.txt
├── stm32f401xe_flash.ld
├── Src/main.cpp
├── Inc/TempSensor.hpp
└── drivers/
    ├── gpio/     # GPIO driver (C++ class + C low-level)
    ├── uart/     # UART driver
    ├── i2c/      # I2C driver
    ├── systick/  # SysTick timer
    ├── rcc/      # Reset & Clock Control
    └── nvic/     # Interrupt controller
```

## Driver Architecture

Two-layer design for each peripheral:
1. **C++ Class Layer**: Object-oriented interface with type safety (C++17)
2. **C Low-level Layer**: Direct hardware register access for performance

## Drivers Overview

### GPIO Driver
Pin configuration and control with multiple modes.
```cpp
GPIO_InitTypeDef cfg{GPIO_PA, GPIO_PIN_5, GPIO_MODE_OUTPUT, GPIO_OTYPER_PP};
GPIO gpio_led{&cfg};
gpio_led.TogglePin(GPIO_PIN_5);
```
**Features**: Input/Output modes, Push-Pull/Open-Drain, Speed control, Pull-up/Pull-down, Alternate functions

### UART Driver
Serial communication at 9600 baud (configurable).
```cpp
USART_InitTypeDef uart_cfg{USART_D2, RX_TX, _9600, USART_CR1_RXNEIE};
UARTDevice uart{&uart_cfg};
uart.Print("Hello, STM32!\r\n", 15);
```
**Features**: Configurable baud rate, interrupt-driven reception, line buffering

### I2C Driver
Non-blocking I2C communication (Standard mode, 100 kHz).
```cpp
I2C_InitTypeDef config{I2C_1, I2C_SPEED_STANDARD, 0, 0, 0, 0};
i2c_device i2c{&config};
i2c.Write(slave_addr, data, size);
while (!i2c.isReady());
i2c.Read(slave_addr, buffer, size);
```
**Features**: Non-blocking operations, state management, timeout handling

### SysTick Timer
Millisecond-precision delays with hardware interrupt.
```cpp
MySysTick(1000);  // Wait 1000 ms
```

## Application: Temperature Sensor

Reads I2C temperature sensor and outputs via UART.

**Features**:
- I2C measurement command & data retrieval
- CRC-8 validation
- Temperature formula: `°C = -45 + (175 × raw_value / 0xFFFF)`
- UART output: `Reading: XX.YY`
- LED (PA5) status indication

## Building & Running

### Prerequisites

- ARM GNU Toolchain (arm-none-eabi-gcc)
- CMake 3.20+
- STM32CubeMX or ST-Link utility for programming
- VS Code with C/C++ extensions (optional)

### Build Steps

```bash
# Clone and configure
git clone https://github.com/ahxiang99/GPIO_Driver_cpp.git
cd GPIO_Driver_cpp
cmake --preset default

# Build
cmake --build build

# Output files
# - build/GPIO_Driver.elf    (ELF executable)
# - build/GPIO_Driver.hex    (Intel HEX)
# - build/GPIO_Driver.bin    (Binary)
```

### Programming

```bash
openocd -f interface/stlink-v2.cfg -f target/stm32f4x.cfg \
  -c "program build/GPIO_Driver.bin 0x08000000 verify reset"
```

## Configuration

### UART (Src/main.cpp)
- Port: USART2 (PA2/PA3) | Baud: 9600 | 8N1

### I2C (Src/main.cpp)
- Interface: I2C1 | Speed: 100 kHz | Pins: PB8 (SCL), PB9 (SDA)

### GPIO
- LED Output: PA5 (Push-Pull, Low Speed)
- I2C Pins: PB8, PB9 (Alternate Function, Open-Drain)

## Memory Usage

- **Firmware Size**: ~29 KB
- **RAM Usage**: < 10 KB (application code)

## Compiler Settings

- **C/C++**: C11 / C++17
- **Optimization**: `-O0` (Debug), `-Os` (Release)
- **Warnings**: `-Wall -Wextra -Wpedantic`
- **Target**: ARM Cortex-M4, Thumb-2, Hardware FPU

## Testing & Debugging

### Serial Monitor
```
Port: /dev/ttyUSB0 (Linux) or COM3 (Windows)
Baud: 9600 | 8N1 | No Flow Control
```

**Expected Output**:
```
Booting...
Reading: 25.43
Reading: 25.42
```

### Hardware Requirements
- ST-Link V2/V2-1 debugger
- USB connection to development board
- Serial terminal for UART output

## API Reference

### GPIO Class
```cpp
class GPIO {
    GPIO(GPIO_InitTypeDef* _cfg);
    GPIO_STATUS InitDriver(GPIO_Config* p_Config);
    GPIO_STATUS TogglePin(const uint16_t PIN);
    bool IsInit();
};
```

### UART Class
```cpp
class UARTDevice {
    USART_Status Print(const char* buffer, uint16_t size);
    USART_Status Get(char* c);
    const char* GetLine();
};
```

### I2C Class
```cpp
class i2c_device {
    I2C_Status Write(uint8_t target_addr, const uint8_t* pData, uint16_t size);
    I2C_Status Read(uint8_t target_addr, uint8_t* pBuffer, uint16_t size);
    bool isReady() const;
};
```

### Temperature Sensor Class
```cpp
class TempSensor {
    TempSensor(II2CMaster& p_Bus);
    float_t GetTemp();
};
```

## Performance

- **GPIO**: < 1 µs response time
- **UART**: 960 bytes/sec @ 9600 baud
- **I2C**: 100 kHz standard mode
- **SysTick**: 1 ms resolution (configurable)

## Troubleshooting

| Issue | Solution |
|-------|----------|
| Device not detected | Verify ST-Link connection, check USB drivers, ensure 3.3V target |
| Compilation errors | Verify ARM toolchain in PATH, CMake ≥ 3.20, C++ headers |
| UART no output | Check baud rate (9600), PA2/PA3 connection, UART clock enabled |
| I2C not working | Verify 4.7kΩ pull-up resistors, slave address (0x88), I2C timing |

## Code Style

- **Classes**: `PascalCase` | **Functions**: `camelCase` | **Constants**: `UPPER_SNAKE_CASE`
- **Private Members**: `m_` prefix
- **Modular Design**: Reusable independent drivers
- **Error Handling**: Status codes for critical operations

## Contributing

Submit PRs for bug fixes, additional drivers (SPI, ADC), optimizations, or documentation.

## License

Licensed under STMicroelectronics STM32 HAL license terms.

## References

- [STM32F401xE Datasheet](https://www.st.com/resource/en/datasheet/stm32f401xe.pdf)
- [STM32F4 Reference Manual](https://www.st.com/resource/en/reference_manual/dm00031020-stm32f405-415-stm32f407-417-stm32f427-437-and-stm32f429-439-advanced-arm-based-32-bit-mcus-stmicroelectroni.pdf)
- [ARM Cortex-M4 User Guide](https://developer.arm.com/documentation/100166/latest)
- [I2C Specification](https://www.nxp.com/docs/en/user-manual/UM10204.pdf)

## Author

**ahxiang99** - Professional embedded systems firmware development project

---

Last Updated: June 2026
