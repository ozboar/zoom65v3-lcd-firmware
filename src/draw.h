#pragma once

#include <stdint.h>

typedef struct {
    uint8_t         width;
    uint8_t         height;
    const uint16_t *palette;
    const uint8_t  *rle;
    uint32_t        rle_size;
} image_t;

/* Glyphs are 2 bits per pixel in rows from the top left */
typedef struct {
    uint8_t         height;
    uint8_t         first;
    uint8_t         last;
    const uint8_t  *widths;
    const uint16_t *offsets;
    const uint8_t  *data;
} font_t;

void    draw_fill(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint16_t color);
void    draw_outline(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint16_t color);
void    draw_image(uint8_t x, uint8_t y, const image_t *image);
/* Draws an image with every color passed through a function */
void    draw_image_mapped(uint8_t x, uint8_t y, const image_t *image, uint16_t (*map)(uint16_t));
uint8_t draw_text_width(const font_t *font, const char *text);
void    draw_text(uint8_t x, uint8_t y, const font_t *font, const char *text, uint16_t fg, uint16_t bg);
