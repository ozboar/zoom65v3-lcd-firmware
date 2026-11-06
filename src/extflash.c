#include "at32f403a_407.h"
#include "extflash.h"
#include "lcd.h"

#define IOMUX_REMAP6 (*(volatile uint32_t *)0x40010030u)
#define FLASH_SELECT (*(volatile uint32_t *)0x40022088u)
#define FLASH_STS3 (*(volatile uint32_t *)0x4002208Cu)
#define FLASH_DA (*(volatile uint32_t *)0x40022098u)

#define WINDOW_START 0x08400000u
#define WINDOW_END 0x08800000u

/* LVGL image descriptor: header, data size, data pointer */
#define DESCRIPTOR_SIZE 12
#define FORMAT_RGB565_ALPHA 5
#define BYTES_PER_PIXEL 3
#define MAX_IMAGE_SIDE 128

void extflash_init(void) {
    gpio_init_type gpio;

    crm_periph_clock_enable(CRM_IOMUX_PERIPH_CLOCK, TRUE);
    crm_periph_clock_enable(CRM_GPIOA_PERIPH_CLOCK, TRUE);
    crm_periph_clock_enable(CRM_GPIOB_PERIPH_CLOCK, TRUE);

    gpio_default_para_init(&gpio);
    gpio.gpio_drive_strength = GPIO_DRIVE_STRENGTH_STRONGER;
    gpio.gpio_out_type       = GPIO_OUTPUT_PUSH_PULL;
    gpio.gpio_mode           = GPIO_MODE_MUX;
    gpio.gpio_pins           = GPIO_PINS_8;
    gpio_init(GPIOA, &gpio);
    gpio.gpio_pins = GPIO_PINS_1 | GPIO_PINS_6 | GPIO_PINS_7 | GPIO_PINS_10 | GPIO_PINS_11;
    gpio_init(GPIOB, &gpio);

    IOMUX_REMAP6 = (IOMUX_REMAP6 & ~0xFu) | 0x9u;

    FLASH_SELECT = 2;
    while (FLASH_STS3 & 1u) {
    }
    FLASH_DA = 0;
}

/* The window only allows word and halfword reads */
static inline uint8_t read_byte(uint32_t address) {
    const uint32_t word = *(const volatile uint32_t *)(address & ~3u);
    return (uint8_t)(word >> ((address & 3u) * 8));
}

static inline uint32_t read_word(uint32_t address) {
    return *(const volatile uint32_t *)address;
}

static bool read_descriptor(uint32_t address, uint8_t *width, uint8_t *height, uint32_t *data) {
    if (address < WINDOW_START || address + DESCRIPTOR_SIZE > WINDOW_END || (address & 3u)) {
        return false;
    }

    const uint32_t header = read_word(address);
    const uint32_t size   = read_word(address + 4);
    const uint32_t w      = (header >> 10) & 0x7FF;
    const uint32_t h      = (header >> 21) & 0x7FF;

    *data = read_word(address + 8);

    if ((header & 0x1F) != FORMAT_RGB565_ALPHA || !w || !h || w > MAX_IMAGE_SIDE || h > MAX_IMAGE_SIDE) {
        return false;
    }
    if (size != w * h * BYTES_PER_PIXEL || *data < WINDOW_START || *data + size > WINDOW_END) {
        return false;
    }

    *width  = (uint8_t)w;
    *height = (uint8_t)h;
    return true;
}

bool extflash_image_size(uint32_t address, uint8_t *width, uint8_t *height) {
    uint32_t data;
    return read_descriptor(address, width, height, &data);
}

/* Pixels are stored high byte first, then alpha */
static uint16_t read_pixel(uint32_t address) {
    const uint16_t color = (uint16_t)((read_byte(address) << 8) | read_byte(address + 1));
    const uint32_t alpha = read_byte(address + 2);
    const uint32_t r     = (color >> 11) * alpha / 255;
    const uint32_t g     = ((color >> 5) & 0x3F) * alpha / 255;
    const uint32_t b     = (color & 0x1F) * alpha / 255;
    return (uint16_t)((r << 11) | (g << 5) | b);
}

uint32_t extflash_read_word(uint32_t address) {
    return read_word(address);
}

static void draw_pixels(uint32_t data, uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t scale) {
    lcd_stream_begin(x, y, x + width * scale - 1, y + height * scale - 1);
    for (uint8_t row = 0; row < height; row++) {
        for (uint8_t pass = 0; pass < scale; pass++) {
            for (uint8_t col = 0; col < width; col++) {
                const uint16_t color = read_pixel(data + ((uint32_t)row * width + (uint32_t)col) * BYTES_PER_PIXEL);
                for (uint8_t repeat = 0; repeat < scale; repeat++) {
                    lcd_stream_pixel(color);
                }
            }
        }
    }
    lcd_stream_end();
}

void extflash_draw_raw(uint32_t address, uint8_t x, uint8_t y, uint8_t width, uint8_t height) {
    draw_pixels(address, x, y, width, height, 1);
}

bool extflash_draw_image(uint32_t address, uint8_t x, uint8_t y, uint8_t scale) {
    uint8_t  width, height;
    uint32_t data;

    if (!read_descriptor(address, &width, &height, &data)) {
        return false;
    }
    draw_pixels(data, x, y, width, height, scale);
    return true;
}
