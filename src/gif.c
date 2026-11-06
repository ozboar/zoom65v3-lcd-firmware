#include "extflash.h"
#include "gif.h"

#include <stdbool.h>

#define DICTIONARY_SIZE 4096
#define COLORS 256
#define TRAILER 0x3B
#define IMAGE_SEPARATOR 0x2C
#define EXTENSION 0x21
#define EXTENSION_CONTROL 0xF9
#define DISPOSE_BACKGROUND 2
#define MIN_DELAY_CENTISECONDS 2
/* Files with no delay play at about the speed stock decodes them */
#define DEFAULT_DELAY_MS 30

/* Above the program and its stack, in SRAM the stock firmware's configuration provides */
#define MEDIA_RAM __attribute__((section(".media_ram")))

static uint16_t canvas[GIF_MAX_SIDE * GIF_MAX_SIDE] MEDIA_RAM;
static uint16_t prefix[DICTIONARY_SIZE] MEDIA_RAM;
static uint8_t  suffix[DICTIONARY_SIZE] MEDIA_RAM;
static uint8_t  stack[DICTIONARY_SIZE] MEDIA_RAM;
static uint16_t global_palette[COLORS] MEDIA_RAM;
static uint16_t local_palette[COLORS] MEDIA_RAM;

static uint32_t position;
static uint32_t end;
static uint32_t frames_start;
static bool     failed;
static bool     has_global_palette;
static uint8_t  width;
static uint8_t  height;

static uint32_t cached_address = 0xFFFFFFFFu;
static uint32_t cached_word;

static uint8_t  dispose;
static uint16_t dispose_x, dispose_y, dispose_w, dispose_h;

static uint8_t next_byte(void) {
    if (position >= end) {
        failed = true;
        return 0;
    }
    const uint32_t aligned = position & ~3u;
    if (aligned != cached_address) {
        cached_address = aligned;
        cached_word    = extflash_read_word(aligned);
    }
    const uint8_t value = (uint8_t)(cached_word >> ((position & 3u) * 8));
    position++;
    return value;
}

static uint16_t next_u16(void) {
    const uint16_t low = next_byte();
    return (uint16_t)(low | (next_byte() << 8));
}

static void read_palette(uint16_t *palette, uint16_t count) {
    for (uint16_t i = 0; i < COLORS; i++) {
        uint32_t r = 0, g = 0, b = 0;
        if (i < count) {
            r = next_byte();
            g = next_byte();
            b = next_byte();
        }
        palette[i] = (uint16_t)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
    }
}

static void skip_sub_blocks(void) {
    while (!failed) {
        const uint8_t size = next_byte();
        if (!size) {
            return;
        }
        position += size;
    }
}

static void fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
    for (uint16_t row = y; row < y + h && row < height; row++) {
        for (uint16_t col = x; col < x + w && col < width; col++) {
            canvas[(uint32_t)row * width + col] = 0;
        }
    }
}

bool gif_open(uint32_t address, uint32_t length) {
    position       = address;
    end            = address + length;
    failed         = false;
    dispose        = 0;
    cached_address = 0xFFFFFFFFu;

    if (next_byte() != 'G' || next_byte() != 'I' || next_byte() != 'F' || next_byte() != '8') {
        return false;
    }
    position += 2;

    const uint16_t w = next_u16();
    const uint16_t h = next_u16();
    if (!w || !h || w > GIF_MAX_SIDE || h > GIF_MAX_SIDE) {
        return false;
    }
    width  = (uint8_t)w;
    height = (uint8_t)h;

    const uint8_t flags = next_byte();
    position += 2;
    has_global_palette = (flags & 0x80) != 0;
    if (has_global_palette) {
        read_palette(global_palette, (uint16_t)(1u << ((flags & 7) + 1)));
    }

    frames_start = position;
    fill_rect(0, 0, width, height);
    return !failed;
}

/* Decodes one image's LZW data onto the canvas. Consumes its sub-blocks */
static bool decode_image(uint16_t fx, uint16_t fy, uint16_t fw, uint16_t fh, bool interlaced, const uint16_t *palette, int16_t transparent) {
    static const uint8_t pass_start[] = {0, 4, 2, 1};
    static const uint8_t pass_step[]  = {8, 8, 4, 2};

    const uint8_t min_size = next_byte();
    if (min_size < 2 || min_size > 8) {
        return false;
    }
    const uint16_t clear = (uint16_t)(1u << min_size);
    const uint16_t stop  = clear + 1;

    for (uint16_t i = 0; i < clear; i++) {
        suffix[i] = (uint8_t)i;
    }

    uint16_t available = stop + 1;
    uint8_t  size      = min_size + 1;
    uint16_t mask      = (uint16_t)((1u << size) - 1);
    int32_t  old       = -1;
    uint8_t  first     = 0;
    uint32_t bits      = 0;
    uint8_t  bit_count = 0;
    uint8_t  block     = 0;
    bool     ended     = false;

    uint16_t x = 0, y = 0;
    uint8_t  pass = 0;
    uint32_t left = (uint32_t)fw * fh;

    while (!failed) {
        while (bit_count < size && !ended) {
            if (!block) {
                block = next_byte();
                if (!block) {
                    ended = true;
                    break;
                }
            }
            bits |= (uint32_t)next_byte() << bit_count;
            bit_count += 8;
            block--;
        }
        if (ended) {
            return !failed;
        }

        uint16_t code = bits & mask;
        bits >>= size;
        bit_count -= size;

        if (code == clear) {
            available = stop + 1;
            size      = min_size + 1;
            mask      = (uint16_t)((1u << size) - 1);
            old       = -1;
            continue;
        }
        if (code == stop) {
            break;
        }

        uint16_t depth = 0;
        if (old < 0) {
            if (code >= clear) {
                return false;
            }
            first        = (uint8_t)code;
            stack[depth++] = first;
            old          = code;
        } else {
            const uint16_t incoming = code;
            if (code > available) {
                return false;
            }
            if (code == available) {
                stack[depth++] = first;
                code           = (uint16_t)old;
            }
            while (code >= clear) {
                if (depth >= DICTIONARY_SIZE - 1) {
                    return false;
                }
                stack[depth++] = suffix[code];
                code           = prefix[code];
            }
            first          = (uint8_t)code;
            stack[depth++] = first;

            if (available < DICTIONARY_SIZE) {
                prefix[available] = (uint16_t)old;
                suffix[available] = first;
                available++;
                if (!(available & mask) && available < DICTIONARY_SIZE) {
                    size++;
                    mask = (uint16_t)((1u << size) - 1);
                }
            }
            old = incoming;
        }

        while (depth && left) {
            const uint8_t index = stack[--depth];
            const uint32_t cx   = (uint32_t)fx + x;
            const uint32_t cy   = (uint32_t)fy + y;
            if (index != transparent && cx < width && cy < height) {
                canvas[cy * width + cx] = palette[index];
            }
            left--;
            if (++x >= fw) {
                x = 0;
                if (interlaced) {
                    y += pass_step[pass];
                    while (y >= fh && pass < 3) {
                        pass++;
                        y = pass_start[pass];
                    }
                } else {
                    y++;
                }
            }
        }
    }

    /* Whatever follows the stop code in this image's data */
    position += block;
    skip_sub_blocks();
    return !failed;
}

bool gif_next_frame(uint32_t *delay_ms) {
    int16_t  transparent = -1;
    uint8_t  next_dispose = 0;
    uint16_t delay       = 0;
    bool     rewound     = false;

    while (!failed) {
        /* The stored length can end before the trailer */
        const uint8_t block = position >= end ? TRAILER : next_byte();

        if (block == EXTENSION) {
            if (next_byte() == EXTENSION_CONTROL) {
                position++;
                const uint8_t flags = next_byte();
                delay               = next_u16();
                const uint8_t index = next_byte();
                next_dispose        = (flags >> 2) & 7;
                transparent         = flags & 1 ? index : -1;
            }
            skip_sub_blocks();
        } else if (block == IMAGE_SEPARATOR) {
            if (dispose == DISPOSE_BACKGROUND) {
                fill_rect(dispose_x, dispose_y, dispose_w, dispose_h);
            }

            const uint16_t x     = next_u16();
            const uint16_t y     = next_u16();
            const uint16_t w     = next_u16();
            const uint16_t h     = next_u16();
            const uint8_t  flags = next_byte();

            const uint16_t *palette = global_palette;
            if (flags & 0x80) {
                read_palette(local_palette, (uint16_t)(1u << ((flags & 7) + 1)));
                palette = local_palette;
            } else if (!has_global_palette) {
                return false;
            }
            if (!w || !h || !decode_image(x, y, w, h, (flags & 0x40) != 0, palette, transparent)) {
                return false;
            }

            dispose   = next_dispose;
            dispose_x = x;
            dispose_y = y;
            dispose_w = w;
            dispose_h = h;

            *delay_ms = delay < MIN_DELAY_CENTISECONDS ? DEFAULT_DELAY_MS : delay * 10u;
            return !failed;
        } else if (block == TRAILER && !rewound) {
            rewound  = true;
            position = frames_start;
            dispose  = 0;
            fill_rect(0, 0, width, height);
        } else {
            return false;
        }
    }
    return false;
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
