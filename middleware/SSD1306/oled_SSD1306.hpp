#pragma once

#include <stdint.h>

#include "RingBuffer.hpp"
#include "cpp/Stm32GpioPin.hpp"
#include "cpp/Stm32Spi.hpp"

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
#define SH1106_DC_PIN GPIO_PIN_6
#define SH1106_DC_H() HAL_GPIO_WritePin(SH1106_DC_PORT, SH1106_DC_PIN, GPIO_PIN_SET)
#define SH1106_DC_L() HAL_GPIO_WritePin(SH1106_DC_PORT, SH1106_DC_PIN, GPIO_PIN_RESET)

// SH1106 RST (Reset) pin (PB14)
#define SH1106_RST_PORT GPIOB
#define SH1106_RST_PIN GPIO_PIN_14
#define SH1106_RST_H() HAL_GPIO_WritePin(SH1106_RST_PORT, SH1106_RST_PIN, GPIO_PIN_SET)
#define SH1106_RST_L() HAL_GPIO_WritePin(SH1106_RST_PORT, SH1106_RST_PIN, GPIO_PIN_RESET)

// SH1106 CS (Chip Select) pin (PB12)
#define SH1106_CS_PORT GPIOB
#define SH1106_CS_PIN GPIO_PIN_12
#define SH1106_CS_H() HAL_GPIO_WritePin(SH1106_CS_PORT, SH1106_CS_PIN, GPIO_PIN_SET)
#define SH1106_CS_L() HAL_GPIO_WritePin(SH1106_CS_PORT, SH1106_CS_PIN, GPIO_PIN_RESET)

// Screen dimensions
#define SCR_W (uint8_t)128  // width
#define SCR_H (uint8_t)64   // height

// SH1106 command definitions
#define SSD1306_CMD_SETMUX (uint8_t)0xA8     // Set multiplex ratio (N, number of lines active on display)
#define SSD1306_CMD_SETOFFS (uint8_t)0xD3    // Set display offset
#define SSD1306_CMD_STARTLINE (uint8_t)0x40  // Set display start line
#define SSD1306_CMD_SEG_NORM (uint8_t)0xA0   // Column 0 is mapped to SEG0 (X coordinate normal)
#define SSD1306_CMD_SEG_INV (uint8_t)0xA1    // Column 127 is mapped to SEG0 (X coordinate inverted)
#define SSD1306_CMD_COM_NORM (uint8_t)0xC0   // Scan from COM0 to COM[N-1] (N - mux ratio, Y coordinate normal)
#define SSD1306_CMD_COM_INV (uint8_t)0xC8    // Scan from COM[N-1] to COM0 (N - mux ratio, Y coordinate inverted)
#define SSD1306_CMD_COM_HW (uint8_t)0xDA     // Set COM pins hardware configuration
#define SSD1306_CMD_CONTRAST (uint8_t)0x81   // Contrast control
#define SSD1306_CMD_EDON (uint8_t)0xA5       // Entire display ON enabled (all pixels on, RAM content ignored)
#define SSD1306_CMD_EDOFF (uint8_t)0xA4      // Entire display ON disabled (output follows RAM content)
#define SSD1306_CMD_INV_OFF (uint8_t)0xA6    // Entire display inversion OFF (normal display)
#define SSD1306_CMD_INV_ON (uint8_t)0xA7     // Entire display inversion ON (all pixels inverted)
#define SSD1306_CMD_CLOCKDIV (uint8_t)0xD5   // Set display clock divide ratio/oscillator frequency
#define SSD1306_CMD_DISP_ON (uint8_t)0xAF    // Display ON
#define SSD1306_CMD_DISP_OFF (uint8_t)0xAE   // Display OFF (sleep mode)

#define SSD1306_CMD_CHARGE_PUMP (uint8_t)0x8D  // Charge pump enable

#define SSD1306_CMD_COL_LOW (uint8_t)0x00    // Set Lower Column Address
#define SSD1306_CMD_COL_HIGH (uint8_t)0x10   // Set Higher Column Address
#define SSD1306_CMD_PAGE_ADDR (uint8_t)0xB0  // Set Page Address

#define SSD1306_CMD_CHARGE (uint8_t)0xD9     //  Dis-charge / Pre-charge Period
#define SSD1306_CMD_SCRL_HR (uint8_t)0x26    // Setup continuous horizontal scroll right
#define SSD1306_CMD_SCRL_HL (uint8_t)0x27    // Setup continuous horizontal scroll left
#define SSD1306_CMD_SCRL_VHR (uint8_t)0x29   // Setup continuous vertical and horizontal scroll right
#define SSD1306_CMD_SCRL_VHL (uint8_t)0x2A   // Setup continuous vertical and horizontal scroll left
#define SSD1306_CMD_SCRL_STOP (uint8_t)0x2E  // Deactivate scroll
#define SSD1306_CMD_SCRL_ACT (uint8_t)0x2F   // Activate scroll

// Entire display on/off enumeration
enum { LCD_ENTIRE_PIXELS_OFF = 0, LCD_ENTIRE_PIXELS_ON = !LCD_ENTIRE_PIXELS_OFF };

// Display pixels inversion enumeration
enum { LCD_INVERT_OFF = 0, LCD_INVERT_ON = !LCD_INVERT_OFF };

// Display ON/OFF enumeration
enum { LCD_OFF = 0, LCD_ON = !LCD_OFF };

// Screen orientation enumeration
enum {
    LCD_ORIENT_NORMAL = 0,  // No rotation
    LCD_ORIENT_CW     = 1,  // Clockwise rotation
    LCD_ORIENT_CCW    = 2,  // Counter-clockwise rotation
    LCD_ORIENT_180    = 3   // 180 degrees rotation
};

// Screen horizontal scroll direction enumeration
enum {
    LCD_SCROLL_RIGHT = 0,  // Scroll right
    LCD_SCROLL_LEFT  = 1   // Scroll left
};

// Screen scroll interval enumeration
enum {
    LCD_SCROLL_IF2   = 0x07,  // 2 frames
    LCD_SCROLL_IF3   = 0x04,  // 3 frames
    LCD_SCROLL_IF4   = 0x05,  // 4 frames
    LCD_SCROLL_IF5   = 0x00,  // 5 frames
    LCD_SCROLL_IF25  = 0x06,  // 25 frames
    LCD_SCROLL_IF64  = 0x01,  // 64 frames
    LCD_SCROLL_IF128 = 0x02,  // 128 frames
    LCD_SCROLL_IF256 = 0x03   // 256 frames
};

// Pixel draw mode
enum {
    LCD_PSET = 0x00,  // Set pixel
    LCD_PRES = 0x01,  // Reset pixel
    LCD_PINV = 0x02   // Invert pixel
};

// Font structure scan lines enumeration
enum {
    FONT_V = (uint8_t)0,         // Vertical font scan lines
    FONT_H = (uint8_t)(!FONT_V)  // Horizontal font scan lines
};

// Font descriptor
typedef struct {
    uint8_t font_Width;        // Width of character
    uint8_t font_Height;       // Height of character
    uint8_t font_BPC;          // Bytes for one character
    uint8_t font_Scan;         // Font scan lines behavior
    uint8_t font_MinChar;      // Code of the first known symbol
    uint8_t font_MaxChar;      // Code of the last known symbol
    uint8_t font_UnknownChar;  // Code of the unknown symbol
    uint8_t font_Data[];       // Font data
} Font_TypeDef;

// Function prototypes

#if (SH1106_USE_DMA)
void SH1106_Flush_DMA(void);
#endif  // SH1106_USE_DMA

void SH1106_ScrollHSetup(uint8_t dir, uint8_t start, uint8_t end, uint8_t interval);
void SH1106_ScrollDSetup(uint8_t dir, uint8_t start, uint8_t end, uint8_t interval, uint8_t voffs);
void SH1106_ScrollStart(void);
void SH1106_ScrollStop(void);

#if (SH1106_OPT_PIXEL)
inline void LCD_Pixel(uint8_t X, uint8_t Y, uint8_t Mode);
#else
void LCD_Pixel(uint8_t X, uint8_t Y, uint8_t Mode);
#endif  // SH1106_OPT_PIXEL

void     LCD_HLine(uint8_t X1, uint8_t X2, uint8_t Y);
void     LCD_VLine(uint8_t X, uint8_t Y1, uint8_t Y2);
void     LCD_Rect(uint8_t X1, uint8_t Y1, uint8_t X2, uint8_t Y2);
void     LCD_FillRect(uint8_t X1, uint8_t Y1, uint8_t X2, uint8_t Y2);
void     LCD_Line(int16_t X1, int16_t Y1, int16_t X2, int16_t Y2);
void     LCD_Circle(int16_t X, int16_t Y, uint8_t R);
void     LCD_Ellipse(uint16_t Xc, uint16_t Yc, uint16_t Ra, uint16_t Rb);

uint8_t  LCD_PutChar(uint8_t X, uint8_t Y, uint8_t Char, const Font_TypeDef* Font);
uint16_t LCD_PutStr(uint8_t X, uint8_t Y, const char* str, const Font_TypeDef* Font);
uint16_t LCD_PutStrLF(uint8_t X, uint8_t Y, const char* str, const Font_TypeDef* Font);
uint8_t  LCD_PutInt(uint8_t X, uint8_t Y, int32_t num, const Font_TypeDef* Font);
uint8_t  LCD_PutIntU(uint8_t X, uint8_t Y, uint32_t num, const Font_TypeDef* Font);
uint8_t  LCD_PutIntF(uint8_t X, uint8_t Y, int32_t num, uint8_t decimals, const Font_TypeDef* Font);
uint8_t  LCD_PutIntLZ(uint8_t X, uint8_t Y, int32_t num, uint8_t digits, const Font_TypeDef* Font);
uint8_t  LCD_PutHex(uint8_t X, uint8_t Y, uint32_t num, const Font_TypeDef* Font);

void     LCD_DrawBitmap(uint8_t X, uint8_t Y, uint8_t W, uint8_t H, const uint8_t* pBMP);
void     LCD_DrawBitmapFullscreen(const uint8_t* pBMP);

class ControlPin {
   public:
    ControlPin(const Stm32GpioPin& pin) : pin_(pin) {
        pin_.Write(GPIO_State::LOW);
    }
    ~ControlPin() {
        pin_.Write(GPIO_State::HIGH);
    }

   private:
    Stm32GpioPin pin_;
};

class OLED_Display {
   public:
    enum class PixelMode : uint8_t { LCD_PSET, LCD_PRES, LCD_PINV };
    enum class OrientMode : uint8_t { NORMAL, CW, CCW, _180 };
    enum class FontColor : uint8_t { BLACK, WHITE };

    typedef struct {
        const uint8_t         width;
        const uint8_t         height;
        const uint16_t* const data;
        const uint8_t* const  char_width;
    } Font_TypeDef;

    OLED_Display();

    void Initialize(const Stm32Spi& spi_);
    void Contrast(uint8_t contrast);
    void SetAllPixelsOn(uint8_t eon_state);
    void SetInvert(uint8_t inv_state);
    void SetDisplayState(uint8_t disp_state);
    void SetXDir(uint8_t x_map);
    void SetYDir(uint8_t y_map);
    void Orientation(OrientMode orientation);

    void Flush(void);
    void FlushPage(uint8_t page);

    void Fill(uint8_t pattern);
    void ClearPage(uint8_t page);

    void DrawBitmapFullscreen(const uint8_t* pBMP);

    /* Draw Function */
    void SetCursor(uint8_t x, uint8_t y);
    void DrawPixel(uint8_t x, uint8_t y, FontColor color);
    char WriteChar(char ch, Font_TypeDef Font, FontColor color);
    char WriteString(char* str, Font_TypeDef Font, FontColor color);

    /* Interface Function */
    void Show(char* buf, uint8_t x, uint8_t y, uint8_t page);

   protected:
    /* Pin Tools */
    void Rst_Pin_L();
    void Rst_Pin_H();
    void DC_Pin_L();
    void DC_Pin_H();
    void CS_Pin_L();
    void CS_Pin_H();

    /* Command Tools */
    void cmd(uint8_t cmd);
    void cmd_double(uint8_t cmd1, uint8_t cmd2);

   private:
    static constexpr GPIO_Config RST_CFG{.pin   = GPIO_PIN_13,
                                         .port  = GPIO_Port::GPIO_PB,
                                         .mode  = GPIO_Moder::GPIO_MODE_OUTPUT,
                                         .otype = GPIO_OType::GPIO_OTYPER_PP,
                                         .ospdr = GPIO_OSPDR::GPIO_OSPEEDR_VHS,
                                         .pupdr = GPIO_PUPDR::GPIO_PUPDR_PULLUP,
                                         .afr   = GPIO_AFR::GPIO_AF0_SYSTEM};
    static constexpr GPIO_Config DC_CFG{.pin   = GPIO_PIN_14,
                                        .port  = GPIO_Port::GPIO_PB,
                                        .mode  = GPIO_Moder::GPIO_MODE_OUTPUT,
                                        .otype = GPIO_OType::GPIO_OTYPER_PP,
                                        .ospdr = GPIO_OSPDR::GPIO_OSPEEDR_VHS,
                                        .pupdr = GPIO_PUPDR::GPIO_PUPDR_PULLUP,
                                        .afr   = GPIO_AFR::GPIO_AF0_SYSTEM};
    static constexpr GPIO_Config CS_CFG{.pin   = GPIO_PIN_15,
                                        .port  = GPIO_Port::GPIO_PB,
                                        .mode  = GPIO_Moder::GPIO_MODE_OUTPUT,
                                        .otype = GPIO_OType::GPIO_OTYPER_PP,
                                        .ospdr = GPIO_OSPDR::GPIO_OSPEEDR_VHS,
                                        .pupdr = GPIO_PUPDR::GPIO_PUPDR_PULLUP,
                                        .afr   = GPIO_AFR::GPIO_AF0_SYSTEM};

    Stm32GpioPin                 Rst_Pin;
    Stm32GpioPin                 DC_Pin;
    Stm32GpioPin                 CS_Pin;
    Stm32Spi                     hspi;

    /* Screen dimensions */
    uint16_t scr_width  = SCR_W;
    uint16_t scr_height = SCR_H;

    uint16_t CurrentX;
    uint16_t CurrentY;

    /* Drawing Mode */
    PixelMode  scr_mode;
    OrientMode scr_orientataion;
};
