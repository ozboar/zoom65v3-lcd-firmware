#include <stdbool.h>

#include "lcd.h"
#include "panel.h"

/* Drawing captured to memory or clipped, and where the next pixel of the current rectangle lands */
static bool     redirected;
static bool     capturing;
static uint8_t *capture_left;
static uint8_t *capture_right;
static uint8_t  capture_split;
static int16_t  view_dx;
static int16_t  clip_x0;
static int16_t  clip_x1;
static int16_t  rect_x0;
static int16_t  rect_x1;
static int16_t  cursor_x;
static uint8_t  cursor_y;
static bool     rect_visible;

void lcd_capture_begin(uint8_t *left, uint8_t *right, uint8_t split) {
    lcd_wait_idle();
    redirected    = true;
    capturing     = true;
    capture_left  = left;
    capture_right = right;
    capture_split = split;
    view_dx       = 0;
    clip_x0       = 0;
    clip_x1       = LCD_WIDTH - 1;
}

void lcd_view_begin(int16_t dx, uint8_t x0, uint8_t x1) {
    redirected = true;
    capturing  = false;
    view_dx    = dx;
    clip_x0    = x0;
    clip_x1    = x1;
}

void lcd_redirect_end(void) {
    redirected = false;
    capturing  = false;
}

void lcd_stream_begin(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1) {
    if (!redirected) {
        panel_window(x0, y0, x1, y1);
        return;
    }
    rect_x0  = x0 + view_dx;
    rect_x1  = x1 + view_dx;
    cursor_x = rect_x0;
    cursor_y = y0;
    if (capturing) {
        return;
    }
    const int16_t left  = rect_x0 > clip_x0 ? rect_x0 : clip_x0;
    const int16_t right = rect_x1 < clip_x1 ? rect_x1 : clip_x1;
    rect_visible        = left <= right;
    if (rect_visible) {
        panel_window((uint8_t)left, y0, (uint8_t)right, y1);
    }
}

static void capture_pixel(uint16_t color) {
    const bool     left   = cursor_x < capture_split;
    uint8_t *const buffer = left ? capture_left : capture_right;

    if (buffer && cursor_y < LCD_HEIGHT) {
        const uint8_t  width  = left ? capture_split : LCD_WIDTH - capture_split;
        const uint16_t column = (uint16_t)(left ? cursor_x : cursor_x - capture_split);
        uint8_t       *pixel  = buffer + ((uint32_t)cursor_y * width + column) * 2;
        pixel[0]              = (uint8_t)(color >> 8);
        pixel[1]              = (uint8_t)color;
    }
}

static void redirected_pixel(uint16_t color) {
    if (cursor_x >= clip_x0 && cursor_x <= clip_x1) {
        if (capturing) {
            capture_pixel(color);
        } else {
            panel_pixel(color);
        }
    }
    if (++cursor_x > rect_x1) {
        cursor_x = rect_x0;
        cursor_y++;
    }
}

void lcd_stream_pixel(uint16_t color) {
    if (redirected) {
        redirected_pixel(color);
    } else {
        panel_pixel(color);
    }
}

void lcd_stream_end(void) {
    if (!redirected || (!capturing && rect_visible)) {
        panel_end();
    }
}

void lcd_send_rect(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, const uint8_t *pixels) {
    if (!redirected) {
        panel_send_rect(x0, y0, x1, y1, pixels);
        return;
    }
    const uint32_t count = (x1 - x0 + 1u) * (y1 - y0 + 1u);
    lcd_stream_begin(x0, y0, x1, y1);
    for (uint32_t i = 0; i < count; i++, pixels += 2) {
        redirected_pixel((uint16_t)((pixels[0] << 8) | pixels[1]));
    }
    lcd_stream_end();
}

void lcd_blit(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, const uint8_t *pixels, uint8_t stride) {
    const uint16_t row_bytes = (x1 - x0 + 1u) * 2u;

    panel_window(x0, y0, x1, y1);
    for (uint8_t y = y0; y <= y1; y++, pixels += stride * 2u) {
        panel_write(pixels, row_bytes);
    }
    panel_end();
}
