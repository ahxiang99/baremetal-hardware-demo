#pragma once

#include <cstdint>
#include <span>

#include "Result.hpp"
#include "RingBuffer.hpp"
#include "cpp/Stm32GpioPin.hpp"
#include "cpp/Stm32Spi.hpp"
#include "cpp/systick.hpp"

// SH1106 display connection:
//   PB12 --> CS
//   PB14 --> RES
//   PC6  --> DC
//   PB13 --> CLK
//   PB15 --> MOSI

// Use bit-banding to draw pixel
//   0 - use logic operations to set pixel color
//   1 - use bit-banding to set pixel color
#define SH1106_USE_BITBAND 0

// Pixel set function definition
//   0 - call pixel function (less code size in cost of speed)
//   1 - inline pixel function (higher speed in cost of code size)
#define SH1106_OPT_PIXEL 1

// DMA usage
//   0 - DMA is not used
//   1 - compile functions for DMA transfer VRAM to display
#define SH1106_USE_DMA 0

// SH1106 HAL

// SPI port
#define SH1106_SPI_PORT hspi2

// GPIO peripherals
#define SH1106_GPIO_PERIPH (RCC_AHBENR_GPIOBEN | RCC_AHBENR_GPIOCEN)

// SH1106 RS/A0 (Data/Command select) pin (PC6)
#define SH1106_DC_PORT GPIOC
#define SH1106_DC_PIN  GPIO_PIN_6
#define SH1106_DC_H()  HAL_GPIO_WritePin(SH1106_DC_PORT, SH1106_DC_PIN, GPIO_PIN_SET)
#define SH1106_DC_L()  HAL_GPIO_WritePin(SH1106_DC_PORT, SH1106_DC_PIN, GPIO_PIN_RESET)

// SH1106 RST (Reset) pin (PB14)
#define SH1106_RST_PORT GPIOB
#define SH1106_RST_PIN  GPIO_PIN_14
#define SH1106_RST_H()  HAL_GPIO_WritePin(SH1106_RST_PORT, SH1106_RST_PIN, GPIO_PIN_SET)
#define SH1106_RST_L()  HAL_GPIO_WritePin(SH1106_RST_PORT, SH1106_RST_PIN, GPIO_PIN_RESET)

// SH1106 CS (Chip Select) pin (PB12)
#define SH1106_CS_PORT GPIOB
#define SH1106_CS_PIN  GPIO_PIN_12
#define SH1106_CS_H()  HAL_GPIO_WritePin(SH1106_CS_PORT, SH1106_CS_PIN, GPIO_PIN_SET)
#define SH1106_CS_L()  HAL_GPIO_WritePin(SH1106_CS_PORT, SH1106_CS_PIN, GPIO_PIN_RESET)

// Screen dimensions
#define SCR_W (uint8_t)128 // width
#define SCR_H (uint8_t)64  // height

// SH1106 command definitions
#define SSD1306_CMD_SETMUX    (uint8_t)0xA8 // Set multiplex ratio (N, number of lines active on display)
#define SSD1306_CMD_SETOFFS   (uint8_t)0xD3 // Set display offset
#define SSD1306_CMD_STARTLINE (uint8_t)0x40 // Set display start line
#define SSD1306_CMD_SEG_NORM  (uint8_t)0xA0 // Column 0 is mapped to SEG0 (X coordinate normal)
#define SSD1306_CMD_SEG_INV   (uint8_t)0xA1 // Column 127 is mapped to SEG0 (X coordinate inverted)
#define SSD1306_CMD_COM_NORM  (uint8_t)0xC0 // Scan from COM0 to COM[N-1] (N - mux ratio, Y coordinate normal)
#define SSD1306_CMD_COM_INV   (uint8_t)0xC8 // Scan from COM[N-1] to COM0 (N - mux ratio, Y coordinate inverted)
#define SSD1306_CMD_COM_HW    (uint8_t)0xDA // Set COM pins hardware configuration
#define SSD1306_CMD_CONTRAST  (uint8_t)0x81 // Contrast control
#define SSD1306_CMD_EDON      (uint8_t)0xA5 // Entire display ON enabled (all pixels on, RAM content ignored)
#define SSD1306_CMD_EDOFF     (uint8_t)0xA4 // Entire display ON disabled (output follows RAM content)
#define SSD1306_CMD_INV_OFF   (uint8_t)0xA6 // Entire display inversion OFF (normal display)
#define SSD1306_CMD_INV_ON    (uint8_t)0xA7 // Entire display inversion ON (all pixels inverted)
#define SSD1306_CMD_CLOCKDIV  (uint8_t)0xD5 // Set display clock divide ratio/oscillator frequency
#define SSD1306_CMD_DISP_ON   (uint8_t)0xAF // Display ON
#define SSD1306_CMD_DISP_OFF  (uint8_t)0xAE // Display OFF (sleep mode)

#define SSD1306_CMD_CHARGE_PUMP (uint8_t)0x8D // Charge pump enable

#define SSD1306_CMD_COL_LOW   (uint8_t)0x00 // Set Lower Column Address
#define SSD1306_CMD_COL_HIGH  (uint8_t)0x10 // Set Higher Column Address
#define SSD1306_CMD_PAGE_ADDR (uint8_t)0xB0 // Set Page Address

#define SSD1306_CMD_CHARGE    (uint8_t)0xD9 //  Dis-charge / Pre-charge Period
#define SSD1306_CMD_SCRL_HR   (uint8_t)0x26 // Setup continuous horizontal scroll right
#define SSD1306_CMD_SCRL_HL   (uint8_t)0x27 // Setup continuous horizontal scroll left
#define SSD1306_CMD_SCRL_VHR  (uint8_t)0x29 // Setup continuous vertical and horizontal scroll right
#define SSD1306_CMD_SCRL_VHL  (uint8_t)0x2A // Setup continuous vertical and horizontal scroll left
#define SSD1306_CMD_SCRL_STOP (uint8_t)0x2E // Deactivate scroll
#define SSD1306_CMD_SCRL_ACT  (uint8_t)0x2F // Activate scroll

// Entire display on/off enumeration
enum {
    LCD_ENTIRE_PIXELS_OFF = 0,
    LCD_ENTIRE_PIXELS_ON = !LCD_ENTIRE_PIXELS_OFF
};

// Display pixels inversion enumeration
enum {
    LCD_INVERT_OFF = 0,
    LCD_INVERT_ON = !LCD_INVERT_OFF
};

// Display ON/OFF enumeration
enum {
    LCD_OFF = 0,
    LCD_ON = !LCD_OFF
};

// Screen orientation enumeration
enum {
    LCD_ORIENT_NORMAL = 0, // No rotation
    LCD_ORIENT_CW = 1,     // Clockwise rotation
    LCD_ORIENT_CCW = 2,    // Counter-clockwise rotation
    LCD_ORIENT_180 = 3     // 180 degrees rotation
};

// Screen horizontal scroll direction enumeration
enum {
    LCD_SCROLL_RIGHT = 0, // Scroll right
    LCD_SCROLL_LEFT = 1   // Scroll left
};

// Screen scroll interval enumeration
enum {
    LCD_SCROLL_IF2 = 0x07,   // 2 frames
    LCD_SCROLL_IF3 = 0x04,   // 3 frames
    LCD_SCROLL_IF4 = 0x05,   // 4 frames
    LCD_SCROLL_IF5 = 0x00,   // 5 frames
    LCD_SCROLL_IF25 = 0x06,  // 25 frames
    LCD_SCROLL_IF64 = 0x01,  // 64 frames
    LCD_SCROLL_IF128 = 0x02, // 128 frames
    LCD_SCROLL_IF256 = 0x03  // 256 frames
};

// Pixel draw mode
enum {
    LCD_PSET = 0x00, // Set pixel
    LCD_PRES = 0x01, // Reset pixel
    LCD_PINV = 0x02  // Invert pixel
};

// Font structure scan lines enumeration
enum {
    FONT_V = (uint8_t)0,        // Vertical font scan lines
    FONT_H = (uint8_t)(!FONT_V) // Horizontal font scan lines
};

// Font descriptor
typedef struct {
    uint8_t font_Width;       // Width of character
    uint8_t font_Height;      // Height of character
    uint8_t font_BPC;         // Bytes for one character
    uint8_t font_Scan;        // Font scan lines behavior
    uint8_t font_MinChar;     // Code of the first known symbol
    uint8_t font_MaxChar;     // Code of the last known symbol
    uint8_t font_UnknownChar; // Code of the unknown symbol
    uint8_t font_Data[];      // Font data
} Font_TypeDef;

// Function prototypes

#if (SH1106_USE_DMA)
void SH1106_Flush_DMA(void);
#endif // SH1106_USE_DMA

void SH1106_ScrollHSetup(uint8_t dir, uint8_t start, uint8_t end, uint8_t interval);
void SH1106_ScrollDSetup(uint8_t dir, uint8_t start, uint8_t end, uint8_t interval, uint8_t voffs);
void SH1106_ScrollStart(void);
void SH1106_ScrollStop(void);

#if (SH1106_OPT_PIXEL)
inline void LCD_Pixel(uint8_t X, uint8_t Y, uint8_t Mode);
#else
void LCD_Pixel(uint8_t X, uint8_t Y, uint8_t Mode);
#endif // SH1106_OPT_PIXEL

void LCD_HLine(uint8_t X1, uint8_t X2, uint8_t Y);
void LCD_VLine(uint8_t X, uint8_t Y1, uint8_t Y2);
void LCD_Rect(uint8_t X1, uint8_t Y1, uint8_t X2, uint8_t Y2);
void LCD_FillRect(uint8_t X1, uint8_t Y1, uint8_t X2, uint8_t Y2);
void LCD_Line(int16_t X1, int16_t Y1, int16_t X2, int16_t Y2);
void LCD_Circle(int16_t X, int16_t Y, uint8_t R);
void LCD_Ellipse(uint16_t Xc, uint16_t Yc, uint16_t Ra, uint16_t Rb);

uint8_t LCD_PutChar(uint8_t X, uint8_t Y, uint8_t Char, const Font_TypeDef *Font);
uint16_t LCD_PutStr(uint8_t X, uint8_t Y, const char *str, const Font_TypeDef *Font);
uint16_t LCD_PutStrLF(uint8_t X, uint8_t Y, const char *str, const Font_TypeDef *Font);
uint8_t LCD_PutInt(uint8_t X, uint8_t Y, int32_t num, const Font_TypeDef *Font);
uint8_t LCD_PutIntU(uint8_t X, uint8_t Y, uint32_t num, const Font_TypeDef *Font);
uint8_t LCD_PutIntF(uint8_t X, uint8_t Y, int32_t num, uint8_t decimals, const Font_TypeDef *Font);
uint8_t LCD_PutIntLZ(uint8_t X, uint8_t Y, int32_t num, uint8_t digits, const Font_TypeDef *Font);
uint8_t LCD_PutHex(uint8_t X, uint8_t Y, uint32_t num, const Font_TypeDef *Font);

void LCD_DrawBitmap(uint8_t X, uint8_t Y, uint8_t W, uint8_t H, const uint8_t *pBMP);
void LCD_DrawBitmapFullscreen(const uint8_t *pBMP);

class ControlPin
{
  public:
    explicit ControlPin(const Stm32GpioPin &pin) : pin_(pin)
    {
	pin_.write(GPIO_State::LOW);
    }
    ~ControlPin()
    {
	pin_.write(GPIO_State::HIGH);
    }

  private:
    Stm32GpioPin pin_;
};

template <uint8_t scr_w, uint8_t scr_h> class ssd1306
{
  public:
    enum class OrientMode : uint8_t {
	NORMAL,
	CW,
	CCW,
	_180
    };

    ssd1306() = default;

    [[nodiscard]] Result<> initialize(const Stm32Spi &spi, const GPIO_Config &RST_CFG, const GPIO_Config &DC_CFG, const GPIO_Config &CS_CFG, const MySysTick &systick)
    {
	spi_ = spi;
	TRY(Rst_Pin.initialize(RST_CFG));
	TRY(DC_Pin.initialize(DC_CFG));
	TRY(CS_Pin.initialize(CS_CFG));

	CS_Pin_H();
	Rst_Pin_L();
	systick.delay_ms(1);
	Rst_Pin_H();
	systick.delay_ms(100);
	CS_Pin_L();
	DC_Pin_L();

	cmd(SSD1306_CMD_DISP_OFF);                 // Display OFF
	cmd_double(SSD1306_CMD_CLOCKDIV, 0x80);    // Set display clock divide ratio/osc freq
	cmd_double(SSD1306_CMD_SETMUX, 0x3F);      // Multiplex ratio (64MUX)
	cmd_double(SSD1306_CMD_SETOFFS, 0x00);     // Display offset = 0
	cmd(SSD1306_CMD_STARTLINE);                // Display start line = 0
	cmd_double(SSD1306_CMD_CHARGE_PUMP, 0x14); // Charge pump enable (SSD1306-specific — NOT 0xAD like SH1106!)
	cmd_double(0x20, 0x00);                    // Memory addressing mode = horizontal
	cmd(SSD1306_CMD_SEG_INV);                  // Segment re-map
	cmd(SSD1306_CMD_COM_INV);                  // COM scan direction (remapped)
	cmd_double(SSD1306_CMD_COM_HW, 0x12);      // COM pins hardware config
	cmd_double(SSD1306_CMD_CONTRAST, 0xCF);    // Contrast
	cmd_double(SSD1306_CMD_CHARGE, 0xF1);      // Pre-charge period
	cmd_double(0xDB, 0x40);                    // VCOMH deselect level
	cmd(SSD1306_CMD_EDOFF);                    // Resume to RAM content
	cmd(SSD1306_CMD_INV_OFF);                  // Normal display
	cmd(SSD1306_CMD_DISP_ON);

	CS_Pin_H();
	return Ok();
    }

    void contrast(const uint8_t contrast)
    {
	ControlPin cs(CS_Pin);
	cmd_double(SSD1306_CMD_CONTRAST, contrast);
    }

    void set_all_pixel_on(const uint8_t eon_state)
    {
	ControlPin cs(CS_Pin);
	cmd(eon_state ? SSD1306_CMD_EDON : SSD1306_CMD_EDOFF);
    }

    void set_invert(const uint8_t inv_state)
    {
	ControlPin cs(CS_Pin);
	cmd(inv_state ? SSD1306_CMD_INV_ON : SSD1306_CMD_INV_OFF);
    }

    void set_disp_state(const uint8_t disp_state)
    {
	ControlPin cs(CS_Pin);
	cmd(disp_state ? SSD1306_CMD_DISP_ON : SSD1306_CMD_DISP_OFF);
    }

    void set_x_dir(const uint8_t x_map)
    {
	ControlPin cs(CS_Pin);
	cmd(x_map ? SSD1306_CMD_SEG_INV : SSD1306_CMD_SEG_NORM);
    }

    void set_y_dir(const uint8_t y_map)
    {
	ControlPin cs(CS_Pin);
	cmd(y_map ? SSD1306_CMD_SEG_INV : SSD1306_CMD_SEG_NORM);
    }

    void set_orientation(OrientMode orientation)
    {
	switch (orientation) {
	case OrientMode::CW: {
	    const uint16_t tmp = scr_width_;
	    scr_width_ = scr_height_;
	    scr_height_ = tmp;
	    set_x_dir(LCD_INVERT_ON);
	    set_y_dir(LCD_INVERT_OFF);
	    break;
	}
	case OrientMode::CCW: {
	    const uint16_t tmp = scr_width_;
	    scr_width_ = scr_height_;
	    scr_height_ = tmp;
	    set_x_dir(LCD_INVERT_OFF);
	    set_y_dir(LCD_INVERT_ON);
	    break;
	}
	case OrientMode::_180:
	    set_x_dir(LCD_INVERT_OFF);
	    set_y_dir(LCD_INVERT_OFF);
	    break;
	default:
	    set_x_dir(LCD_INVERT_ON);
	    set_y_dir(LCD_INVERT_ON);
	    break;
	}
	scr_orientation_ = orientation;
    }

    void flush(const std::span<const uint8_t, ((scr_w * scr_h) >> 3)> frame_buffer)
    {
	ControlPin cs(CS_Pin);
	const uint32_t bits_h = scr_h >> 3;
	uint8_t ram_pointer[] = {SSD1306_CMD_COL_LOW, SSD1306_CMD_COL_HIGH, 0};
	uint8_t *page_addr = &ram_pointer[2];
	for (uint32_t page = 0; page < bits_h; ++page) {
	    (*page_addr) = SSD1306_CMD_PAGE_ADDR | page;
	    DC_Pin_L();
	    spi_.transferOnly((uint8_t *)ram_pointer, sizeof(ram_pointer));
	    const uint8_t *data_ptr = &frame_buffer[page * scr_w];
	    DC_Pin_H();
	    spi_.transferOnly(data_ptr, scr_w);
	}
    }

    void flush_page(const std::span<const uint8_t, scr_w> frame_buffer, const uint8_t page)
    {
	const uint8_t ram_pointer[] = {SSD1306_CMD_COL_LOW, SSD1306_CMD_COL_HIGH, static_cast<uint8_t>(SSD1306_CMD_PAGE_ADDR | page)};
	DC_Pin_L();
	spi_.transferOnly(ram_pointer, sizeof(ram_pointer));
	DC_Pin_H();
	spi_.transferOnly(&frame_buffer[page * scr_w], scr_w);
    }

    void cmd(const uint8_t cmd)
    {
	DC_Pin_L();
	spi_.transferOnly(&cmd, sizeof(cmd));
    }
    void cmd_double(const uint8_t cmd1, const uint8_t cmd2)
    {
	DC_Pin_L();
	const uint8_t command[2]{cmd1, cmd2};
	spi_.transferOnly(command, sizeof(command));
    }

  protected:
    void Rst_Pin_L()
    {
	Rst_Pin.write(GPIO_State::LOW);
    }
    void Rst_Pin_H()
    {
	Rst_Pin.write(GPIO_State::HIGH);
    }
    void DC_Pin_L()
    {
	DC_Pin.write(GPIO_State::LOW);
    }
    void DC_Pin_H()
    {
	DC_Pin.write(GPIO_State::HIGH);
    }
    void CS_Pin_L()
    {
	CS_Pin.write(GPIO_State::LOW);
    }
    void CS_Pin_H()
    {
	CS_Pin.write(GPIO_State::HIGH);
    }

  private:
    Stm32GpioPin Rst_Pin;
    Stm32GpioPin DC_Pin;
    Stm32GpioPin CS_Pin;
    Stm32Spi spi_;

    uint16_t scr_width_{scr_w};
    uint16_t scr_height_{scr_h};
    OrientMode scr_orientation_{};
};
