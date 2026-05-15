# STM32F401XE GPIO & Peripheral Driver

A comprehensive C++ embedded systems firmware project for the STM32F401XE Cortex-M4 microcontroller. This project provides object-oriented drivers for GPIO, UART, I2C, and SysTick interfaces, with a practical application demonstrating I2C temperature sensor integration.

## Project Overview

This project demonstrates professional embedded systems development practices with:

- **Modern C++ (C++17)** for hardware abstraction and object-oriented design
- **C-level low-level drivers** for efficient hardware access
- **CMake build system** for cross-platform compilation
- **VS Code integration** with proper linting and debugging configuration
- **I2C temperature sensor** application with CRC validation

### Target Hardware

- **MCU**: STM32F401XE (ARM Cortex-M4)
- **Flash**: 512 KB
- **RAM**: 96 KB
- **Clock**: 84 MHz
- **FPU**: Hardware floating-point unit

## Project Structure

```
GPIO_Driver_cpp/
├── CMakeLists.txt              # CMake build configuration
├── CMakePresets.json           # CMake presets for VS Code
├── stm32f401xe_flash.ld        # Linker script for STM32F401XE
├── GPIO_Driver_CPP.code-workspace  # VS Code workspace configuration
│
├── Src/
│   └── main.cpp                # Main application entry point
│
├── Inc/
│   ├── TempSensor.hpp          # Temperature sensor header
│   └── TempSensor.cpp          # Temperature sensor implementation
│
├── drivers/
│   ├── gpio/
│   │   ├── cpp/
│   │   │   ├── gpio.hpp        # C++ GPIO class
│   │   │   └── gpio.cpp        # GPIO implementation
│   │   ├── low-level/
│   │   │   ├── gpio.h          # Low-level GPIO interface
│   │   │   ├── gpio.c          # GPIO hardware abstraction
│   │   │   └── gpio_types.h    # GPIO type definitions
│   │
│   ├── uart/
│   │   ├── cpp/
│   │   │   ├── uart.hpp        # C++ UART class
│   │   │   └── uart.cpp        # UART implementation
│   │   └── low-level/
│   │       ├── uart.h          # Low-level UART interface
│   │       ├── uart.c          # UART hardware abstraction
│   │       └── uart_types.h    # UART type definitions
│   │
│   ├── i2c/
│   │   ├── cpp/
│   │   │   ├── i2c.hpp         # C++ I2C class
│   │   │   └── i2c.cpp         # I2C implementation
│   │   └── low-level/
│   │       ├── i2c.h           # Low-level I2C interface
│   │       ├── i2c.c           # I2C hardware abstraction
│   │       └── i2c_types.h     # I2C type definitions
│   │
│   ├── systick/
│   │   ├── cpp/
│   │   │   └── systick.cpp     # SysTick timer implementation
│   │   └── low-level/
│   │       └── systick_types.h # SysTick type definitions
│   │
│   ├── rcc/                    # Reset & Clock Control
│   ├── nvic/                   # Nested Vectored Interrupt Controller
│   └── common/                 # Common definitions
│
└── cmake/
    └── vscode_generated.cmake  # VS Code CMake generation
```

## Driver Architecture

### Layered Design

Each peripheral driver follows a two-layer architecture:

1. **C++ Class Layer** (High-level abstraction)
   - Object-oriented interface
   - Type safety with C++17
   - Resource management

2. **C Low-level Layer** (Hardware abstraction)
   - Direct hardware register access
   - Optimized for performance
   - Reusable across projects

### GPIO Driver

Provides pin configuration and control with support for multiple GPIO modes.

```cpp
GPIO_InitTypeDef cfg{GPIO_PA, GPIO_PIN_5, GPIO_MODE_OUTPUT, 
                     GPIO_OTYPER_PP, GPIO_OSPEEDR_LS, 
                     GPIO_PUPDR_NOPULL, 0};
GPIO gpio_led{&cfg};

// Toggle LED
gpio_led.TogglePin(GPIO_PIN_5);
```

**Features:**
- Pin mode configuration (Input, Output, Alternate Function)
- Output type selection (Push-Pull, Open-Drain)
- Speed control (Low Speed, Medium Speed, High Speed, Very High Speed)
- Pull-up/Pull-down configuration
- Alternate function mapping

### UART Driver

Serial communication interface for data transmission and reception.

```cpp
USART_InitTypeDef uart_cfg{USART_D2, RX_TX, _9600, USART_CR1_RXNEIE};
UARTDevice uart{&uart_cfg};

// Print to serial
char message[] = "Hello, STM32!\r\n";
uart.Print(message, strlen(message));
```

**Features:**
- Configurable baud rate (tested at 9600)
- Interrupt-driven reception
- Line buffering for string input
- Character and string transmission

### I2C Driver

Inter-Integrated Circuit communication with non-blocking operations.

```cpp
I2C_InitTypeDef config{I2C_1, I2C_SPEED_STANDARD, 0, 0, 0, 0};
i2c_device i2c{&config};

// Non-blocking I2C operations
i2c.Write(slave_addr, data, size);
while (!i2c.isReady());  // Wait for completion
i2c.Read(slave_addr, buffer, size);
```

**Features:**
- Standard mode (100 kHz) I2C
- Non-blocking read/write operations
- State management for bus status
- Timeout handling for bus fault recovery

### SysTick Timer

System tick timer for millisecond-precision delays.

```cpp
MySysTick(1000);  // Wait 1000 milliseconds
```

**Features:**
- Hardware SysTick interrupt
- Millisecond resolution
- Non-blocking design

## Application: Temperature Sensor

The main application demonstrates all drivers working together to read temperature data from an I2C-connected sensor.

### Features

- **I2C Communication**: Sends measurement command and reads temperature data
- **CRC-8 Validation**: Validates data integrity with checksums
- **Temperature Conversion**: Converts raw sensor value to Celsius
- **Serial Output**: Transmits readings via UART at 9600 baud
- **Status Indication**: LED (PA5) toggles to show active operation

### Temperature Calculation

```
Temperature (°C) = -45 + (175 × raw_value / 0xFFFF)
```

### Data Format

Temperature data is transmitted via UART in the format:
```
Reading: XX.YY
```

Where XX is the integer part and YY is the decimal part (2 fractional digits).

## Building and Running

### Prerequisites

- **ARM GNU Toolchain** (arm-none-eabi-gcc)
- **CMake** 3.20 or later
- **STM32CubeMX** or similar for programming
- **VS Code** with C/C++ extensions (optional)

### Build Steps

1. **Clone the repository**
   ```bash
   git clone https://github.com/ahxiang99/GPIO_Driver_cpp.git
   cd GPIO_Driver_cpp
   ```

2. **Configure with CMake**
   ```bash
   cmake --preset default
   ```

3. **Build the project**
   ```bash
   cmake --build build
   ```

4. **Output files**
   - `build/GPIO_Driver.elf` - ELF executable
   - `build/GPIO_Driver.hex` - Intel HEX format
   - `build/GPIO_Driver.bin` - Binary format
   - `build/GPIO_Driver.map` - Memory map

### Programming the Device

Using ST-Link utility or your preferred programmer:

```bash
# Using OpenOCD (example)
openocd -f interface/stlink-v2.cfg -f target/stm32f4x.cfg \
  -c "program build/GPIO_Driver.bin 0x08000000 verify reset"
```

## Configuration

### UART Configuration

Edit `Src/main.cpp` to modify UART settings:
- Port: USART2 (PA2/PA3)
- Baud Rate: 9600
- Data Bits: 8
- Stop Bits: 1
- Parity: None

### I2C Configuration

Edit `Src/main.cpp` for I2C settings:
- Interface: I2C1
- Speed: 100 kHz (Standard mode)
- Pins: PB8 (SCL), PB9 (SDA)

### GPIO Configuration

Modify GPIO initialization in `main.cpp`:
- **LED Output**: PA5 (Push-Pull, Low Speed)
- **I2C Pins**: PB8, PB9 (Alternate Function, Open-Drain)

## Memory Usage

### Flash
- Compiled firmware size: ~29 KB

### RAM
- Stack and heap allocation configurable via linker script
- Typical usage: < 10 KB for application code

## Compiler Settings

- **C Standard**: C11
- **C++ Standard**: C++17
- **Optimization**: `-O0` (Debug), `-Os` (Release)
- **Warnings**: `-Wall -Wextra -Wpedantic`
- **Target**: ARM Cortex-M4, Thumb-2 instruction set
- **FPU**: Single-precision hardware floating-point

## Development Environment

### VS Code Extensions Recommended

- C/C++ (Microsoft)
- Cortex-Debug
- CMake Tools

### Formatting

Code is formatted with Clang-Format (C++11 style):
- 4-space indentation
- Column limit: 80
- See `.clang-format` for full configuration

## Testing and Debugging

### Hardware Requirements

- ST-Link V2/V2-1 debugger
- USB connection to development board
- Serial terminal (for UART output)

### Serial Monitor Connection

```
Port: /dev/ttyUSB0 (Linux) or COM3 (Windows)
Baud Rate: 9600
Data Bits: 8
Stop Bits: 1
Parity: None
Flow Control: None
```

Expected output:
```
Booting...
Reading: 25.43
Reading: 25.42
Reading: 25.44
...
```

## Code Style

This project follows professional embedded systems coding standards:

- **Naming Conventions**: 
  - Classes: `PascalCase`
  - Functions: `camelCase`
  - Constants: `UPPER_SNAKE_CASE`
  - Private members: `m_` prefix

- **Comments**: Clear documentation of complex logic
- **Modular Design**: Drivers are independent and reusable
- **Error Handling**: Status codes for critical operations

## API Reference

### GPIO Class

```cpp
class GPIO {
    GPIO(GPIO_InitTypeDef* _cfg);
    GPIO_STATUS InitDriver(GPIO_Config* p_Config);
    GPIO_STATUS ResetDriver();
    GPIO_STATUS SetPinConfig(GPIO_Config* p_Config);
    GPIO_STATUS TogglePin(const uint16_t PIN);
    bool IsInit();
};
```

### UART Class

```cpp
class UARTDevice {
    UARTDevice(USART_InitTypeDef* cfg);
    USART_Status DataAvailable();
    USART_Status Print(const char* buffer, uint16_t size);
    USART_Status Get(char* c);
    const char* GetLine();
    bool HandleInput();
};
```

### I2C Class

```cpp
class i2c_device : public II2CMaster {
    i2c_device(I2C_InitTypeDef* pConfig);
    I2C_Status Write(uint8_t target_addr, const uint8_t* pData, uint16_t size);
    I2C_Status Read(uint8_t target_addr, uint8_t* pBuffer, uint16_t size);
    bool isReady() const;
    I2C_State GetState() const;
    void SetState(I2C_State pState);
};
```

### Temperature Sensor Class

```cpp
class TempSensor {
    TempSensor(II2CMaster& p_Bus);
    float_t GetTemp();
};
```

## Performance Characteristics

- **GPIO Response Time**: < 1 µs
- **UART Throughput**: 960 bytes/sec at 9600 baud
- **I2C Speed**: 100 kHz standard mode
- **SysTick Resolution**: 1 ms (configurable)
- **FPU Performance**: Full hardware acceleration for floating-point

## Troubleshooting

### Issue: Device not detected
- Verify ST-Link connection
- Check USB drivers on host computer
- Ensure correct target voltage (3.3V)

### Issue: Compilation errors
- Ensure ARM GNU Toolchain is installed and in PATH
- Verify CMake version >= 3.20
- Check C++ standard library headers

### Issue: UART data not appearing
- Verify baud rate matches (9600)
- Check PA2/PA3 pins are properly connected
- Ensure UART peripheral is enabled in clock control

### Issue: I2C not communicating
- Verify pull-up resistors on SCL/SDA (typical: 4.7 kΩ)
- Check slave device address matches (0x88 for temp sensor)
- Inspect I2C timing configuration

## Contributing

Feel free to fork this project and submit pull requests for:
- Bug fixes
- Additional peripheral drivers (SPI, ADC, etc.)
- Performance optimizations
- Documentation improvements

## License

This project includes code generated by STMicroelectronics STM32CubeIDE and is provided under the same license terms as the STM32 HAL libraries.

## References

- [STM32F401xE Datasheet](https://www.st.com/resource/en/datasheet/stm32f401xe.pdf)
- [STM32F4 Reference Manual](https://www.st.com/resource/en/reference_manual/dm00031020-stm32f405-415-stm32f407-417-stm32f427-437-and-stm32f429-439-advanced-arm-based-32-bit-mcus-stmicroelectronics.pdf)
- [ARM Cortex-M4 Generic User Guide](https://developer.arm.com/documentation/100166/latest)
- [I2C Specification](https://www.nxp.com/docs/en/user-manual/UM10204.pdf)

## Author

**ahxiang99**

Created as a professional embedded systems development project demonstrating best practices in microcontroller firmware development.

---

Last Updated: May 2026
