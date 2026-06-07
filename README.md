# STM32F401XE Bare-Metal Firmware Demo

A production-grade embedded firmware project for the **STM32F401XE** microcontroller featuring modular C++17 drivers with DMA support, interrupt handling, and hardware peripherals integration. This project demonstrates professional embedded systems development practices with clean architecture separation and reusable components.

## Target Hardware

| Component | Specification |
|-----------|---------------|
| **MCU** | STM32F401XE (ARM Cortex-M4, 84 MHz) |
| **Flash** | 512 KB |
| **RAM** | 96 KB |
| **FPU** | Hardware (ARMv7 single-precision) |
| **Debug** | SWD (ST-Link compatible) |

## Project Overview

This firmware implements a complete embedded system with:
- **Multi-peripheral driver architecture** (GPIO, UART, I2C, Timer, SysTick)
- **Dual-layer design**: C++ class interface + C low-level register access
- **DMA acceleration** for UART and I2C communication
- **I2C temperature/humidity sensor** (SHT40) integration
- **Command-line interface** (CLI) for interactive control
- **Configurable middleware** (logger, CLI, callbacks)
- **Hardware interrupt management** with vector routing

## Language Composition

```
C++       50.2% │███████████████████████████ (Drivers, applications)
C         33.6% │████████████████████         (Low-level drivers)
Assembly   8.9% │█████                        (Startup, context)
CMake      4.3% │██                           (Build system)
Linker      3%  │██                           (Memory layout)
```

## Project Structure

```
baremetal-hardware-demo/
├── CMakeLists.txt                      # Build configuration
├── stm32f401xe_flash.ld               # Linker script (Flash layout)
│
├── Src/
│   ├── main.cpp                        # Application entry point
│   ├── startup_stm32f401xx.S          # Cortex-M4 startup code
│   ├── sysmem.c                        # Newlib heap management
│   └── syscall.c                       # POSIX system calls
│
├── Inc/
│   ├── Sht40ad1b.hpp                   # SHT40 sensor driver
│   ├── SHT4X.hpp                       # SHT4X variant support
│   ├── Sensor.hpp                      # Sensor interface
│   ├── logger.hpp                      # Logging macros
│   └── FloatIntExtraction.hpp          # Float formatting utility
│
├── drivers/
│   ├── gpio/                           # GPIO controller
│   │   ├── cpp/                        # C++ class wrappers
│   │   │   ├── IGpio.hpp              # GPIO interface
│   │   │   ├── Stm32GpioPin.hpp       # GPIO pin controller
│   │   │   └── Stm32GpioPin.cpp
│   │   └── low-level/                  # Direct register access
│   │       ├── gpio.h
│   │       ├── gpio_types.h
│   │       ├── gpio_registers.h
│   │       └── gpio.c
│   │
│   ├── uart/                           # UART/USART interface
│   │   ├── cpp/
│   │   │   ├── IUart.hpp              # UART interface
│   │   │   ├── Stm32Uart.hpp/cpp      # Basic UART driver
│   │   │   ├── InterruptUart.hpp/cpp  # Interrupt-driven
│   │   │   └── DmaUart.hpp/cpp        # DMA-accelerated
│   │   ├── low-level/
│   │   │   ├── uart.h
│   │   │   ├── uart_types.h
│   │   │   └── uart_registers.h
│   │   └── common/
│   │       └── RegisterUtils.hpp       # Bitfield utilities
│   │
│   ├── i2c/                            # I2C bus controller
│   │   ├── cpp/
│   │   │   ├── II2C.hpp               # I2C interface
│   │   │   ├── Stm32I2C.hpp/cpp       # Basic I2C driver
│   │   │   ├── InterruptI2C.hpp/cpp   # Interrupt-driven
│   │   │   └── DmaI2C.hpp/cpp         # DMA-accelerated
│   │   └── low-level/
│   │       ├── i2c.h
│   │       ├── i2c_types.h
│   │       └── i2c_registers.h
│   │
│   ├── timer/                          # TIM3 peripheral
│   │   ├── cpp/
│   │   │   ├── ITimer.hpp             # Timer interface
│   │   │   └── Stm32Timer.hpp/cpp     # Timer implementation
│   │   └── low-level/
│   │       ├── tim.h
│   │       ├── tim_types.h
│   │       ├── tim_registers.h
│   │       └── tim.c
│   │
│   ├── systick/                        # System tick timer
│   │   ├── cpp/
│   │   │   ├── ISysTick.hpp           # SysTick interface
│   │   │   └── systick.hpp/cpp        # 1ms ticker
│   │   └── low-level/
│   │       └── systick.c
│   │
│   ├── rcc/                            # Reset & Clock Control
│   │   ├── cpp/
│   │   │   └── Rcc.hpp
│   │   └── low-level/
│   │       ├── rcc.h
│   │       └── rcc.c
│   │
│   ├── nvic/                           # Interrupt controller
│   │   └── low-level/
│   │       └── nvic.h
│   │
│   ├── DMA/                            # Direct Memory Access
│   │   ├── cpp/
│   │   │   └── Dma.hpp/cpp
│   │   └── low-level/
│   │       └── dma.h
│   │
│   └── common/
│       └── RegisterUtils.hpp           # Bit manipulation macros
│
├── Middleware/
│   ├── cli.hpp/cpp                     # Command-line interface
│   └── logger.hpp/cpp                  # Debug logging
│
└── docs/
    └── (project documentation)
```

## Driver Architecture

### Two-Layer Design Pattern

Each peripheral follows a clean separation of concerns:

```
┌─────────────────────────────────────┐
│  C++ Class Layer (Object-Oriented)  │  Type-safe, RAII, STL
├─────────────────────────────────────┤
│ Hardware Abstraction Layer (HAL)    │  State management, scheduling
├─────────────────────────────────────┤
│  C Low-Level Layer (Register I/O)   │  Direct hardware control
├─────────────────────────────────────┤
│     STM32F401 Peripheral Regs       │  Memory-mapped I/O
└─────────────────────────────────────┘
```

### Key Design Principles

- **Separation of Concerns**: Low-level register access isolated from application logic
- **Interface-Based Design**: Abstract interfaces (`IGpio`, `IUart`, `II2C`) enable testing and modularity
- **Resource Management**: RAII pattern ensures proper initialization/cleanup
- **Type Safety**: C++17 enums and strong types prevent configuration errors
- **Performance**: Inline register access in hot paths; DMA for bulk transfers
- **Callback-Based Events**: Decoupled interrupt handlers and data processing

## Peripheral Drivers

### GPIO (General Purpose I/O)

Pin I/O control with flexible configuration.

```cpp
// Configure PA5 as push-pull output, low speed
GPIO_Config led_cfg;
led_cfg.port  = GPIO_Port::GPIO_PA;
led_cfg.pin   = GPIO_PIN_5;
led_cfg.mode  = GPIO_Moder::GPIO_MODE_OUTPUT;
led_cfg.otype = GPIO_OType::GPIO_OTYPER_PP;
led_cfg.ospdr = GPIO_OSPDR::GPIO_OSPEEDR_LS;

Stm32GpioPin gpio_led(GPIOA, led_cfg);
gpio_led.Init();
gpio_led.Toggle();
```

**Features**:
- Input/Output modes
- Push-Pull / Open-Drain configuration
- Speed control (LS, MS, HS, VHS)
- Pull-up / Pull-down resistors
- Alternate function mapping

### UART (Serial Communication)

Full-duplex asynchronous serial communication with DMA support.

```cpp
// Configure USART2 @ 9600 baud, TX-only (TX_ONLY or RX_TX)
UartConfig uart_cfg;
uart_cfg.dev_num  = UartNum::USART_D2;
uart_cfg.baudRate = UartBaudRate::_9600;
uart_cfg.comm     = UartComm::TX_ONLY;
uart_cfg.parity   = UartParity::NONE;
uart_cfg.stopbits = UartStopBit::STOP_1;

DmaUart uart(USART2, uart_cfg);
uart.initialize();
uart.send((uint8_t*)"Hello\r\n", 7);
```

**Implementation Options**:
- **Polling**: Synchronous (blocking) transfers
- **Interrupt-Driven**: Callback-based reception
- **DMA**: Hardware-accelerated transfers (production)

**Features**:
- Configurable baud rate (9600, 115200 supported)
- Independent TX/RX configuration
- Parity control (None, Even, Odd)
- 1-2 stop bits
- Error detection (framing, overrun, parity)

### I2C (Inter-Integrated Circuit)

Multi-master synchronous serial bus with DMA capability.

```cpp
// Configure I2C1 @ 100 kHz standard mode
I2C_Config i2c_cfg;
i2c_cfg.ClockFreq      = I2C_Clk_Freq::_100KHz;
i2c_cfg.AddressingMode = I2C_Addressing_Mode::AddressMode_7Bit;

DmaI2C i2c(I2C1, i2c_cfg);
i2c.initialize();

// Non-blocking write
i2c.Write(0x88, cmd_buffer, 1);
while (!i2c.isReady());

// Non-blocking read
i2c.Read(0x88, data_buffer, 6);
```

**Implementation Options**:
- **Polling**: Check status register
- **Interrupt-Driven**: ISR-based state machine
- **DMA**: Full DMA-based transfers (production)

**Features**:
- 7-bit and 10-bit addressing
- Standard (100 kHz) and Fast (400 kHz) modes
- Non-blocking operations with state tracking
- Timeout protection
- Clock stretching support

### Timer (TIM3)

Programmable interval timer with interrupt support.

```cpp
TimerConfig timer_cfg;
timer_cfg.Instance          = APB1_TIMER_3;
timer_cfg.CounterMode       = TIM_COUNTERMODE_UP;
timer_cfg.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;

Stm32Timer timer(TIM3, timer_cfg);
timer.start(500);  // 500ms period

if (timer.isElapsed()) {
    // Execute periodic task
    timer.start(500);
}
```

**Features**:
- Up/down counting modes
- Configurable reload value
- Interrupt generation
- Period calculation

### SysTick Timer

System tick interrupt for precise timing (1ms resolution).

```cpp
extern MySysTick my_systick;

my_systick.init();           // Initialize 1ms ticker
uint32_t ticks = my_systick.get_ticks();
my_systick.delay_ms(1000);   // Wait 1 second
```

**Features**:
- 1 millisecond resolution
- 32-bit tick counter (overflow after ~49 days)
- Hardware exception handler
- Precise timing for sensor polling

## Application: Temperature & Humidity Sensor

### SHT40 Sensor Integration

Reads I2C-based SHT40 sensor (temperature -45..85°C, humidity 0..100%).

```cpp
// Global sensor instance using I2C1 bus
Sht40ad1b temp_sensor(i2c1, "SHT40");

// In main loop:
temp_sensor.read();           // Initiate measurement
temp_sensor.ProcessData();    // Process received data

// In callback (when I2C data received):
if (temp_sensor.getState() == SensorState::WAIT_DATA) {
    temp_sensor.setState(SensorState::DATA_READY);
}
```

### Sensor Features

- **I2C Address**: 0x89
- **Measurement Command**: 0xFD (high precision)
- **Conversion Time**: ~30ms
- **Data Format**: 2-byte temperature + CRC, 2-byte humidity + CRC
- **Temperature Calculation**: `°C = -45 + (175 × raw_value / 0xFFFF)`
- **Humidity Calculation**: `%RH = -6 + (125 × raw_value / 0xFFFF)` (clamped 0-100%)
- **CRC Validation**: CRC-8 polynomial validation

### State Machine

```
IDLE → MEASURING (issue I2C write) 
    → WAIT_DATA (wait ~30ms) 
    → DATA_READY (receive I2C data) 
    → process & log 
    → IDLE
```

## Building & Running

### Prerequisites

- **ARM GNU Toolchain**: `arm-none-eabi-gcc` >= 10.3
- **CMake**: >= 3.20
- **ST-Link Utility** or OpenOCD for programming
- **Serial Terminal**: PuTTY, minicom, or VS Code extension

### Build Steps

```bash
# Clone repository
git clone https://github.com/ahxiang99/baremetal-hardware-demo.git
cd baremetal-hardware-demo

# Configure with CMake preset
cmake --preset default

# Build project
cmake --build build

# Outputs
ls build/
# - baremetal-hardware-demo.elf    (ELF executable)
# - baremetal-hardware-demo.hex    (Intel HEX format)
# - baremetal-hardware-demo.bin    (Raw binary)
```

### Programming the Firmware

#### Using OpenOCD + GDB

```bash
# Terminal 1: Start OpenOCD server
openocd -f interface/stlink-v2.cfg -f target/stm32f4x.cfg

# Terminal 2: Program and debug
arm-none-eabi-gdb build/baremetal-hardware-demo.elf
(gdb) target remote localhost:3333
(gdb) load
(gdb) continue
```

#### Using STM32CubeProgrammer (GUI)

1. Connect ST-Link V2 to development board
2. Open STM32CubeProgrammer
3. Select `build/baremetal-hardware-demo.bin`
4. Click "Download" (set start address: 0x08000000)

#### Using OpenOCD (One-Shot)

```bash
openocd -f interface/stlink-v2.cfg -f target/stm32f4x.cfg \
  -c "program build/baremetal-hardware-demo.bin 0x08000000 verify reset"
```

## Configuration

### System Configuration (main.cpp)

```cpp
#define CLI_ENABLE    0    // Command-line interface (set 1 to enable)
#define SENSOR_ENABLE 1    // Temperature sensor (set 0 to disable)
```

### Peripheral Configuration

| Peripheral | Setting | Value |
|------------|---------|-------|
| **UART2** | Port | PA2 (RX), PA3 (TX) |
| | Baud Rate | 9600 |
| | Format | 8N1 (8 data, no parity, 1 stop) |
| **I2C1** | Pins | PB8 (SCL), PB9 (SDA) |
| | Speed | 100 kHz (Standard mode) |
| | Addressing | 7-bit |
| **SysTick** | Resolution | 1 ms |
| **TIM3** | Mode | Up-counting |
| **GPIO** | LED Output | PA5 (active high) |

### Clock Configuration

- **HSI (Internal Oscillator)**: 16 MHz
- **PLL Multiplier**: 10.5x → 84 MHz system clock
- **AHB Prescaler**: /1 → 84 MHz AHB
- **APB1 Prescaler**: /2 → 42 MHz (timers ×2 = 84 MHz)
- **APB2 Prescaler**: /1 → 84 MHz

## Performance Characteristics

| Component | Metric | Value |
|-----------|--------|-------|
| **GPIO** | Toggle latency | < 1 µs |
| **UART** | Throughput @ 9600 | 960 bytes/sec |
| **UART** | Throughput @ 115200 | 11.5 kB/sec |
| **I2C** | Clock frequency | 100 kHz (standard) |
| **I2C** | Byte transfer time | ~80 µs per byte |
| **SysTick** | Timer resolution | 1 ms |
| **Firmware Size** | Total | ~35-40 KB |
| **RAM Usage** | Runtime | < 10 KB |

## Memory Map

```
0x08000000  ┌─────────────────────────────────┐
            │     .text (Code)                │
            │     .rodata (Constants)         │  ~30-35 KB
            │     .data (Initialized vars)    │
            ├─────────────────────────────────┤
            │     (Unused Flash)              │
0x0807FFFF  └─────────────────────────────────┘

0x20000000  ┌─────────────────────────────────┐
            │     .data (RAM copy)            │
            │     .bss (Uninitialized)        │  < 10 KB
            │     Heap (malloc)               │
            ├─────────────────────────────────┤
            │     Stack (grows downward)      │  4 KB reserved
0x20018000  └─────────────────────────────────┘
```

## Compiler & Optimization Settings

```cmake
# C/C++ Standards
CMAKE_C_STANDARD:       11
CMAKE_CXX_STANDARD:     20
CMAKE_CXX_EXTENSIONS:   OFF

# Architecture
Target CPU:             Cortex-M4
ISA:                    Thumb-2
FPU:                    ARMv7 (single-precision)
Float ABI:              Hard

# Optimization (Debug)
Flags:                  -O0 -g3 -Wall -Wextra -Wpedantic

# Optimization (Release)
Flags:                  -Os -Wall -Wextra -Wpedantic
```

## Testing & Debugging

### Serial Monitor Output

Connect to the UART interface and observe:

```
Port:       /dev/ttyUSB0 (Linux) or COM3 (Windows)
Baud:       9600
Format:     8N1
No Flow Control
```

**Expected Output (SENSOR_ENABLE=1, CLI_ENABLE=0)**:

```
Booting...
System Initialized via IUart interface!
USART2 Initialized
I2C1 Initialized
Initialized Done...
Temp: 25.43
Rh: 45.23
Temp: 25.44
Rh: 45.20
...
```

### Hardware Debugging

**GDB Debugging**:

```bash
# Terminal 1
openocd -f interface/stlink-v2.cfg -f target/stm32f4x.cfg

# Terminal 2
arm-none-eabi-gdb build/baremetal-hardware-demo.elf
(gdb) target remote :3333
(gdb) load
(gdb) break main
(gdb) continue
(gdb) step          # Single step
(gdb) print var     # Inspect variable
```

### Hardware Requirements

- **Debug Probe**: ST-Link V2 or V2-1
- **Target Board**: STM32F401 Nucleo or custom board
- **Power**: 3.3V (50-100mA typical)
- **Connection**: USB for ST-Link, serial cable optional
- **Serial Terminal**: For UART output monitoring

## Troubleshooting

| Issue | Cause | Solution |
|-------|-------|----------|
| Device not detected | ST-Link connection issue | Check USB cable, verify board has 3.3V |
| Compilation error: "arm-none-eabi-gcc not found" | Toolchain not in PATH | Install ARM GNU Toolchain, add to system PATH |
| Baud rate mismatch | Serial terminal wrong speed | Set to 9600 baud, 8N1 |
| I2C not working | Missing pull-up resistors | Add 4.7kΩ resistors on SCL/SDA to 3.3V |
| I2C slave not responding | Wrong I2C address or clock issue | Verify sensor address (0x89 for SHT40), check PB8/PB9 connections |
| No UART output | UART clock disabled | Verify RCC clock enable for USART2, check PA2/PA3 |
| Hard fault during boot | Memory/stack corruption | Check linker script, enable hard fault debugger breakpoint |

## Code Style & Conventions

| Element | Style | Example |
|---------|-------|---------|
| **Classes** | PascalCase | `Stm32GpioPin`, `DmaUart` |
| **Functions** | camelCase | `initialize()`, `handleInterrupt()` |
| **Constants** | UPPER_SNAKE_CASE | `GPIO_PIN_5`, `USART_D2` |
| **Enums** | PascalCase | `UartBaudRate`, `GPIO_Port` |
| **Private Members** | `m_` prefix | `m_pInstance`, `m_State` |
| **Comments** | Doxygen style | `/** @brief ... */` |
| **Max Line Length** | 100 characters | For readability |

## Extension Points

This firmware is designed for easy extension:

- **Add SPI Driver**: Implement `ISpi` interface + low-level code
- **Add ADC Driver**: Follow `IGpio` pattern in `drivers/adc/`
- **Custom Sensors**: Extend `Sensor` base class, use I2C/UART interface
- **Enhanced Logging**: Modify `logger.hpp` for file output or network
- **RTOS Integration**: Replace cooperative scheduling with FreeRTOS
- **USB Support**: Add STM32 USB library following existing patterns

## Contributing

Contributions are welcome! Please:

1. Follow the two-layer architecture pattern
2. Use C++17 features (no C++20+ yet)
3. Write doxygen-style comments
4. Test with hardware before submitting PR
5. Update documentation for new features

## License

This project is provided under the **STMicroelectronics STM32 HAL license terms** and compatible with open-source development.

## References & Resources

- [STM32F401XE Datasheet](https://www.st.com/resource/en/datasheet/stm32f401xe.pdf) - Complete hardware specifications
- [STM32F4 Reference Manual](https://www.st.com/resource/en/reference_manual/dm00031020-stm32f405-415-stm32f407-417-stm32f427-437-and-stm32f429-439-advanced-arm-based-32-bit-mcus-stmicroelectroni.pdf) - Peripheral details
- [ARM Cortex-M4 User Guide](https://developer.arm.com/documentation/100166/latest) - CPU architecture
- [I2C Specification](https://www.nxp.com/docs/en/user-manual/UM10204.pdf) - I2C protocol (NXP)
- [SHT40 Sensor Datasheet](https://sensirion.com/products/catalog/SHT40/) - Temperature/humidity sensor
- [OpenOCD Documentation](http://openocd.org/) - JTAG/SWD debugging

## Project Statistics

- **Total LOC**: ~8,000-10,000
- **Driver Count**: 7 (GPIO, UART, I2C, Timer, SysTick, RCC, NVIC)
- **Implementation Variants**: 9 (polling, interrupt, DMA versions)
- **Test Coverage**: Hardware integration tested
- **Build Time**: ~2-3 seconds (incremental)

## Author

**ahxiang99** — Professional embedded systems firmware development  
GitHub: [@ahxiang99](https://github.com/ahxiang99)

---

**Last Updated**: June 2026  
**Status**: Production-Ready ✓
