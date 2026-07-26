// init.cpp
#include "Result.hpp"
#include "board_config.hpp"
#include "cpp/DmaUart.hpp"
#include "cpp/InterruptUart.hpp"
#include "cpp/Stm32GpioPin.hpp"
#include "drivers.hpp"
#include "logger.hpp"
#include "pch.hpp"
#include <typeinfo>

namespace
{

enum class BootTag : uint8_t {
	Clock = 1,
	GpioMux,
	Led,
	Uart2,
	I2c1,
	Exti,
	Rtc,
	Spi1,
	Oled
};

template <typename T1, typename T2, typename T3, typename T4>
void initializeUart(T1 &uart, T2 &&cfg, T3 &&tx_cfg, T4 &&rx_cfg)
{
	if constexpr (std::is_same_v<T1, DmaUart>) {
		checkOk(uart.initialize(cfg, tx_cfg, rx_cfg), BootTag::Uart2);
	} else if constexpr (std::is_same_v<T1, InterruptUart>) {
		checkOk(uart.initialize(cfg), BootTag::Uart2);
	}
}

[[noreturn]] void panic(BootTag tag, uint32_t err)
{
	__disable_irq(); // no scheduler, no ISRs from here on

	// 1. Breadcrumb first — works even if UART is the casualty.
	_PWR->CR |= PWR_CR_DBP;
	RTC->BKP0R[5] = 0xDEAD0000u | (static_cast<uint32_t>(tag) << 8) | (err & 0xFF);

	// 2. Best-effort raw UART. Polled TXE, no DMA, no Logger.
	//    Safe even pre-init: TE-not-set just means TXE never fires → timeout.
	auto putc = [](char c) {
		for (uint32_t t = 0; t < 50'000; ++t) { // bounded, never hangs
			if (USART2->SR & USART_SR_TXE) {
				USART2->DR = c;
				return;
			}
		}
	};
	const char msg[] = "\r\nPANIC tag=";
	for (const char *p = msg; *p; ++p) {
		putc(*p);
	}
	putc('0' + static_cast<uint8_t>(tag));

	auto pulse = [](bool on, uint32_t cycles) {
		GPIOA->BSRR = on ? GPIO_PIN_5 : (GPIO_PIN_5 << 16);
		for (volatile uint32_t i = 0; i < cycles; ++i) {
		}
	};

	// 3. SOS on LD2 via raw registers — no driver object needed.
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOA_EN;
	(void)RCC->AHB1ENR;
	GPIOA->MODER = (GPIOA->MODER & ~(3u << 10)) | (1u << 10); // PA5 output
	constexpr uint32_t DOT = 150'000, DASH = 450'000, GAP = 450'000, WORD_GAP = 2'000'000;
	while (true) {
		for (int i = 0; i < 3; ++i) {
			pulse(true, DOT);
			pulse(false, GAP);
		} // S
		for (int i = 0; i < 3; ++i) {
			pulse(true, DASH);
			pulse(false, GAP);
		} // O
		for (int i = 0; i < 3; ++i) {
			pulse(true, DOT);
			pulse(false, GAP);
		} // S
		pulse(false, WORD_GAP);
	}
}

void checkOk(const Result<> &r, BootTag what)
{
	if (!r.isOk()) {
		panic(what, static_cast<uint32_t>(r.error()));
	}
}

void enableFpu()
{
	// Enable FPU by setting bits 20, 21, 22, and 23
	static volatile uint32_t &CPACR = *reinterpret_cast<volatile uint32_t *>(0xE000ED88);
	CPACR |= ((3UL << 20) | (3UL << 22));
	// Manual Barrier        instructions(Assembly)
	__asm volatile("dsb 0xf" ::: "memory");
	__asm volatile("isb 0xf" ::: "memory");
}

void initCore(Drivers &g)
{
	enableFpu();
	g.sysclock.initialize(board::sys_cfg_84, board::clock_tree);
	g.my_systick.init();
	LOG_INFO("AHB: {}, APB1: {}, APB2: {}", board::clock_tree.ahb, board::clock_tree.apb1,
		 board::clock_tree.apb2);
}

void initComms(Drivers &g)
{
	initializeUart(g.uart2, board::uart2::cfg, board::uart2::hdmatx_cfg,
		       board::uart2::hdmarx_cfg);
	Logger::Init(UartRef::from(g.uart2));
	Logger::set_level(LogLevel::INFO);
	LOG_INFO("UART2 up");

	initializeUart(g.uart1, board::uart1::cfg, nullptr, nullptr);

	checkOk(g.i2c1.initialize(board::i2c1::cfg), BootTag::I2c1);
	LOG_INFO("I2C1 up");
}

void initGpioMux()
{
	/* this is for peripherals gpio */
	static constexpr const GPIO_Config *mux[] = {
		&board::uart2::gpio_cfg, &board::uart1::gpio_cfg, &board::i2c1::gpio_cfg,
		&board::spi1::gpio_cfg};
	for (auto *cfg : mux) {
		checkOk(Gpio::configureMux(*cfg), BootTag::GpioMux);
	}
}

void initIo(Drivers &g)
{
	checkOk(g.gpio_led.initialize(board::led::gpio_cfg), BootTag::Led);
}

void initDisplay(Drivers &g)
{
	checkOk(g.spi1.initialize(board::spi1::cfg), BootTag::Spi1);
	LOG_INFO("SPI up");
	checkOk(g.disp.Initialize(g.spi1), BootTag::Oled);
	LOG_INFO("OLED up");
}

void initTimer(Drivers &g)
{
	g.timer.initialize(board::tim3::cfg);
	LOG_INFO("TIM3 Initialized");
}

void initExti(Drivers &g)
{
	/* GPIO C Pin 13 */
	g.user_button.initialize(board::exti::pc13_gpio_cfg);
	LOG_INFO("PC13 (Button) EXTI Up");
}

void initRtc(Drivers &g)
{
	/* Setup RTC Clock */
	if (!g.rtc.IsInit()) {
		RTC_TimeTypeDef t{.Hours = 15, .Minutes = 53, .Seconds = 0, .TimeFormat = 0};
		RTC_DateTypeDef d{.WeekDay = 1, .Year = 26, .Month = 7, .Day = 14};
		g.rtc.setTime(t);
		g.rtc.setDate(d);
	}
	checkOk(g.rtc.initialize(), BootTag::Rtc);
	LOG_INFO("RTC Up");
}
} // namespace

void initDriver(Drivers &g)
{
	initCore(g);
	initGpioMux();
	initComms(g);
	initIo(g);
	initExti(g);
	initTimer(g);
	initRtc(g);
	initDisplay(g);
	LOG_INFO("Boot complete");
}
