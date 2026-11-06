#pragma once

#include <stdbool.h>
#include <stdint.h>

#define GIF_MAX_SIDE 118

/* Opens a GIF stored at a flash address, and returns false if it is not usable */
bool gif_open(uint32_t address, uint32_t length);

/* Decodes the next frame onto the canvas, looping at the end. Returns false on bad data */
bool gif_next_frame(uint32_t *delay_ms);

uint8_t         gif_width(void);
uint8_t         gif_height(void);
const uint16_t *gif_canvas(void);
