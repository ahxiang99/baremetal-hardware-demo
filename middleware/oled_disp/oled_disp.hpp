//
// Created by ahxia on 8/8/2026.
//

#ifndef BAREMETAL_HARDWARE_DEMO_OLED_DISP_H
#define BAREMETAL_HARDWARE_DEMO_OLED_DISP_H

#include "Result.hpp"
#include "RingBuffer.hpp"
#include "SSD1306/SSD1306.hpp"
#include "cpp/Stm32GpioPin.hpp"
#include "cpp/Stm32Spi.hpp"
#include "cpp/systick.hpp"
#include <array>
#include <span>
#include <cstdint>

namespace Font
{
enum class Color : uint8_t {
    BLACK,
    WHITE
};

struct Font_TypeDef {
    const uint8_t width;
    const uint8_t height;
    const uint16_t *const data;
    const uint8_t *const char_width;
};
} // namespace Font

class oled_disp
{
  public:
    oled_disp() = default;

    [[nodiscard]] Result<> initialize(const Stm32Spi &spi, const GPIO_Config &rst_cfg, const GPIO_Config &dc_cfg, const GPIO_Config &cs_cfg, const MySysTick &systick);

    Result<> flush_page(std::span<const uint8_t>, uint8_t);
    void flush_page(uint8_t);
    void display_on();
    void display_off();

    void show(char *, uint8_t, uint8_t, uint8_t);
    void draw_pixel(uint8_t, uint8_t);
    char write_char(char ch, Font::Font_TypeDef, Font::Color);
    char write_string(char *str, Font::Font_TypeDef, Font::Color);

    void clear_page(uint8_t);
    void fill_page(uint8_t, uint8_t);

    void set_cursor(uint8_t, uint8_t);
    void set_contrast(uint8_t);
    void set_all_pixel_on(uint8_t);
    void set_invert(uint8_t);
    void set_display_state(uint8_t);
    void set_X_dir(uint8_t);
    void set_Y_dir(uint8_t);
    void set_orientation(uint8_t);

  private:
    void draw_pixel(uint8_t, uint8_t, Font::Color);

    ssd1306<128, 64> disp_;
    std::array<uint8_t, ((128 * 64) >> 3)> vram_{};
    uint16_t screen_width_{};
    uint16_t screen_height_{};
    uint16_t CurrentX{};
    uint16_t CurrentY{};
};

#endif // BAREMETAL_HARDWARE_DEMO_OLED_DISP_H
