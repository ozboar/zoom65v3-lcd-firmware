#pragma once

#include <stdbool.h>
#include <stdint.h>

/* Visible area, in screen coordinates from the top left */
#define LCD_WIDTH 118
#define LCD_HEIGHT 118

#define RGB565(r, g, b) ((uint16_t)((((r) >> 3) << 11) | (((g) >> 2) << 5) | ((b) >> 3)))

void lcd_init(void);
void lcd_set_power(bool on);
void lcd_set_backlight(bool on);

/* Backlight brightness, 0 to 100 */
void lcd_set_brightness(uint8_t percent);

/* Starts writing a screen rectangle. Pixels go left to right, then top to bottom */
void lcd_stream_begin(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1);
void lcd_stream_pixel(uint16_t color);
void lcd_stream_end(void);
