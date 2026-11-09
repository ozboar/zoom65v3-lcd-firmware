#pragma once

#include <stdint.h>

/* The panel itself, which lcd_target.c draws through. Pixels are written into the window left to right, then top to bottom */
void panel_window(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1);
void panel_pixel(uint16_t color);
void panel_write(const uint8_t *bytes, uint16_t len);
void panel_end(void);

/* Starts a DMA transfer of big endian pixels and returns, see lcd_send_rect */
void panel_send_rect(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, const uint8_t *pixels);
