#pragma once

#include <stdbool.h>
#include <stdint.h>

/* Stock images and uploads on the screen's external flash. Uploads are written by media.c */
void extflash_init(void);

/* Word and halfword reads only. Also reads the internal flash */
uint32_t extflash_read_word(uint32_t address);

/* Draws raw stock pixels, 3 bytes each, with no descriptor */
void extflash_draw_raw(uint32_t address, uint8_t x, uint8_t y, uint8_t width, uint8_t height);

/*
 * Draws a stock image at a whole number scale, with its top left at (x, y). Transparent
 * parts blend to black. Returns false if the address does not hold a valid image.
 */
bool extflash_draw_image(uint32_t address, uint8_t x, uint8_t y, uint8_t scale);

/* Reads a stock image's size. Returns false if the address does not hold a valid image */
bool extflash_image_size(uint32_t address, uint8_t *width, uint8_t *height);
