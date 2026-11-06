#include "draw.h"
#include "lcd.h"

void draw_fill(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint16_t color) {
    uint32_t count = (uint32_t)(x1 - x0 + 1) * (uint32_t)(y1 - y0 + 1);

    lcd_stream_begin(x0, y0, x1, y1);
    while (count--) {
        lcd_stream_pixel(color);
    }
    lcd_stream_end();
}

void draw_outline(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint16_t color) {
    draw_fill(x0, y0, x1, y0, color);
    draw_fill(x0, y1, x1, y1, color);
    draw_fill(x0, y0, x0, y1, color);
    draw_fill(x1, y0, x1, y1, color);
}

void draw_image(uint8_t x, uint8_t y, const image_t *image) {
    draw_image_mapped(x, y, image, 0);
}

void draw_image_mapped(uint8_t x, uint8_t y, const image_t *image, uint16_t (*map)(uint16_t)) {
    lcd_stream_begin(x, y, x + image->width - 1, y + image->height - 1);
    for (uint32_t i = 0; i + 1 < image->rle_size; i += 2) {
        uint16_t color = image->palette[image->rle[i + 1]];
        if (map) {
            color = map(color);
        }
        for (uint8_t run = image->rle[i]; run > 0; run--) {
            lcd_stream_pixel(color);
        }
    }
    lcd_stream_end();
}

static uint16_t blend(uint16_t fg, uint16_t bg, uint8_t level) {
    const uint8_t inverse = 3 - level;
    uint16_t      r       = ((fg >> 11) * level + (bg >> 11) * inverse) / 3;
    uint16_t      g       = (((fg >> 5) & 0x3F) * level + ((bg >> 5) & 0x3F) * inverse) / 3;
    uint16_t      b       = ((fg & 0x1F) * level + (bg & 0x1F) * inverse) / 3;
    return (uint16_t)((r << 11) | (g << 5) | b);
}

uint8_t draw_text_width(const font_t *font, const char *text) {
    uint8_t width = 0;
    for (; *text; text++) {
        if ((uint8_t)*text >= font->first && (uint8_t)*text <= font->last) {
            width += font->widths[(uint8_t)*text - font->first];
        }
    }
    return width;
}

void draw_text(uint8_t x, uint8_t y, const font_t *font, const char *text, uint16_t fg, uint16_t bg) {
    const uint16_t colors[4] = {bg, blend(fg, bg, 1), blend(fg, bg, 2), fg};

    for (; *text; text++) {
        uint8_t code = (uint8_t)*text;
        if (code < font->first || code > font->last) {
            continue;
        }

        const uint8_t  width = font->widths[code - font->first];
        const uint8_t *data  = font->data + font->offsets[code - font->first];

        lcd_stream_begin(x, y, x + width - 1, y + font->height - 1);
        for (int16_t col = width - 1; col >= 0; col--) {
            for (uint8_t row = 0; row < font->height; row++) {
                uint32_t pixel = (uint32_t)row * width + (uint32_t)col;
                uint8_t  level = (data[pixel / 4] >> ((pixel % 4) * 2)) & 3;
                lcd_stream_pixel(colors[level]);
            }
        }
        lcd_stream_end();
        x += width;
    }
}
