#include <stdbool.h>

/* Selects the library's C API, and word copies, which this core can do unaligned */
#define PICO_BUILD
#ifdef __arm__
#    define ARM_MATH_CM4
#endif

#include "gif.h"

#define GIF_TURBO_GAP 8
#define GIF_TURBO_MAX_PIXELS (GIF_MAX_SIDE * GIF_MAX_SIDE)

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wall"
#pragma GCC diagnostic ignored "-Wextra"
#include "AnimatedGIF.h"
#include "gif.inl"
#pragma GCC diagnostic pop

#include "extflash.h"

#define DISPOSE_BACKGROUND 2
#define INTERLACED 0x40
#define HAS_TRANSPARENCY 1

/* Above the program and its stack, in SRAM the stock firmware's configuration provides */
#define MEDIA_RAM __attribute__((section(".media_ram")))

static uint16_t canvas[GIF_MAX_SIDE * GIF_MAX_SIDE] MEDIA_RAM;
static GIFIMAGE decoder MEDIA_RAM;
/* The output pixels, then the same again as room for its overrun and code tables */
static uint8_t  turbo[TURBO_BUFFER_SIZE + GIF_TURBO_MAX_PIXELS + GIF_TURBO_GAP] __attribute__((aligned(4))) MEDIA_RAM;

static uint32_t file_address;
static uint8_t  width;
static uint8_t  height;
static uint8_t  dispose;
static uint16_t dispose_x, dispose_y, dispose_w, dispose_h;

/* The flash only reads words */
static int32_t read_file(GIFFILE *file, uint8_t *out, int32_t length) {
    if (length > file->iSize - file->iPos) {
        length = file->iSize - file->iPos;
    }
    if (length <= 0) {
        return 0;
    }

    uint32_t address = file_address + (uint32_t)file->iPos;
    for (int32_t left = length; left > 0;) {
        if (!(address & 3u) && !((uintptr_t)out & 3u) && left >= 4) {
            *(uint32_t *)out = extflash_read_word(address);
            out += 4;
            address += 4;
            left -= 4;
        } else {
            *out++ = (uint8_t)(extflash_read_word(address & ~3u) >> ((address & 3u) * 8));
            address++;
            left--;
        }
    }
    file->iPos += length;
    return length;
}

static int32_t seek_file(GIFFILE *file, int32_t position) {
    file->iPos = position < 0 ? 0 : position >= file->iSize ? file->iSize - 1 : position;
    return file->iPos;
}

static void fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
    for (uint16_t row = y; row < y + h && row < height; row++) {
        for (uint16_t col = x; col < x + w && col < width; col++) {
            canvas[(uint32_t)row * width + col] = 0;
        }
    }
}

bool gif_open(uint32_t address, uint32_t length) {
    GIF_begin(&decoder, GIF_PALETTE_RGB565_LE);
    decoder.pfnRead       = read_file;
    decoder.pfnSeek       = seek_file;
    decoder.GIFFile.iSize = (int32_t)length;
    decoder.pTurboBuffer  = turbo;
    file_address          = address;

    if (!GIFInit(&decoder) || decoder.iCanvasWidth > GIF_MAX_SIDE || decoder.iCanvasHeight > GIF_MAX_SIDE) {
        return false;
    }
    width   = (uint8_t)decoder.iCanvasWidth;
    height  = (uint8_t)decoder.iCanvasHeight;
    dispose = 0;
    fill_rect(0, 0, width, height);
    return true;
}

/* The row a line of the output goes on, which for interlaced images isn't the same order */
static int output_row(int line) {
    if (!(decoder.ucMap & INTERLACED)) {
        return line;
    }
    const int last = decoder.iHeight - 1;
    if (line > last / 2) {
        return line * 2 - (last | 1);
    }
    if (line > last / 4) {
        return line * 4 - ((last & ~1) | 2);
    }
    return line > last / 8 ? line * 8 - ((last & ~3) | 4) : line * 8;
}

static bool decode_frame(int *delay) {
    decoder.iError = GIF_SUCCESS;
    GIF_playFrame(&decoder, delay, 0);
    return decoder.iError == GIF_SUCCESS;
}

bool gif_next_frame(uint32_t *delay_ms) {
    bool restarting = decoder.GIFFile.iPos >= decoder.GIFFile.iSize - 1;
    int  delay      = 0;

    /* Whatever follows the last frame, such as padding, ends the loop rather than being an error */
    if (!decode_frame(&delay)) {
        if (restarting) {
            return false;
        }
        decoder.GIFFile.iPos = 0;
        restarting           = true;
        if (!decode_frame(&delay)) {
            return false;
        }
    }

    if (restarting) {
        fill_rect(0, 0, width, height);
    } else if (dispose == DISPOSE_BACKGROUND) {
        fill_rect(dispose_x, dispose_y, dispose_w, dispose_h);
    }

    const uint16_t *palette     = decoder.bUseLocalPalette ? decoder.pLocalPalette : decoder.pPalette;
    const bool      transparent = decoder.ucGIFBits & HAS_TRANSPARENCY;
    const uint8_t   key         = decoder.ucTransparent;
    const uint8_t  *in          = turbo;
    const int       x           = decoder.iX;
    const int       visible     = x >= width ? 0 : (x + decoder.iWidth > width ? width - x : decoder.iWidth);

    for (int line = 0; line < decoder.iHeight; line++, in += decoder.iWidth) {
        const int row = output_row(line) + decoder.iY;
        if (row >= height) {
            continue;
        }
        uint16_t *out = &canvas[(uint32_t)row * width + x];
        for (int i = 0; i < visible; i++) {
            if (!transparent || in[i] != key) {
                out[i] = palette[in[i]];
            }
        }
    }

    dispose   = (decoder.ucGIFBits >> 2) & 7;
    dispose_x = decoder.iX;
    dispose_y = decoder.iY;
    dispose_w = decoder.iWidth;
    dispose_h = decoder.iHeight;

    *delay_ms = delay;
    return true;
}

uint8_t gif_width(void) {
    return width;
}

uint8_t gif_height(void) {
    return height;
}

const uint16_t *gif_canvas(void) {
    return canvas;
}
