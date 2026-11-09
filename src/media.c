#include "assets.h"
#include "at32f403a_407.h"
#include "board.h"
#include "crc.h"
#include "draw.h"
#include "extflash.h"
#include "gif.h"
#include "lcd.h"
#include "media.h"
#include "ui.h"

#define CHANNEL_NONE 0
#define CHANNEL_IMAGE 1
#define CHANNEL_GIF 2

#define COMMAND_DELETE_IMAGE 0xE0
#define COMMAND_DELETE_GIF 0xE1

/* Stock locations, see the stock firmware's external flash map */
#define IMAGE_FLAG 0x08600000u
#define GIF_FLAG 0x08601000u
#define GIF_LENGTH 0x08603000u
#define IMAGE_START 0x08610000u
#define IMAGE_END 0x08620000u
#define GIF_START 0x08620000u
#define GIF_END 0x08800000u
#define FLAG_SET 0x11111111u
#define SECTOR_SIZE 0x1000u

/* Stock defaults, kept in the internal flash */
#define DEFAULT_GIF 0x0803C480u
#define DEFAULT_GIF_LENGTH 12650u
#define DEFAULT_IMAGE 0x0803F9BCu

#define IMAGE_SIDE 110
#define IMAGE_BYTES (IMAGE_SIDE * IMAGE_SIDE * 3u)
#define PACKET_DATA 24u
#define PACKET_HEADER 2u
#define PACKET_CRC 4u
#define MAX_GIF_BYTES (0x10000u * PACKET_DATA)
#define UPLOAD_TIMEOUT_MS 5000

#define MESSAGE_Y 53

static uint8_t  channel = CHANNEL_NONE;
static bool     length_set;
static uint32_t total;
static uint32_t index_next;
static uint32_t erased_until;
static uint32_t activity;

static bool     page_gif;
static bool     page_drawn;
static bool     gif_ready;
static uint32_t frame_timer;
static uint32_t frame_delay;
static bool     margins_stale;
static bool     page_changed;

/* Every write goes through these two. Nothing outside the upload slots and their flags can change */
static bool writable(uint32_t address) {
    return address == IMAGE_FLAG || address == GIF_FLAG || address == GIF_LENGTH || (address >= IMAGE_START && address < GIF_END);
}

static bool erase_sector(uint32_t address) {
    if ((address & (SECTOR_SIZE - 1)) || !writable(address)) {
        return false;
    }
    flash_spim_unlock();
    const flash_status_type status = flash_sector_erase(address);
    flash_spim_lock();
    return status == FLASH_OPERATE_DONE;
}

static bool program_word(uint32_t address, uint32_t value) {
    if ((address & 3u) || !writable(address)) {
        return false;
    }
    flash_spim_unlock();
    const flash_status_type status = flash_word_program(address, value);
    flash_spim_lock();
    return status == FLASH_OPERATE_DONE;
}

bool media_upload_active(void) {
    return channel != CHANNEL_NONE;
}

bool media_upload_receiving(void) {
    return channel != CHANNEL_NONE && length_set;
}

uint16_t media_next_index(void) {
    return (uint16_t)index_next;
}

/* Only the last packet can be short. A short GIF packet is padded to keep the CRC aligned */
uint8_t media_packet_length(void) {
    const uint32_t left = total - index_next * PACKET_DATA;
    const uint32_t data = left < PACKET_DATA ? left : PACKET_DATA;
    return (uint8_t)(PACKET_HEADER + (channel == CHANNEL_GIF ? (data + 3u) & ~3u : data) + PACKET_CRC);
}

void media_upload_cancel(void) {
    if (channel != CHANNEL_NONE) {
        channel = CHANNEL_NONE;
        ui_upload_end();
    }
}

bool media_upload_start(uint8_t requested) {
    if (requested != CHANNEL_IMAGE && requested != CHANNEL_GIF) {
        return false;
    }
    channel    = requested;
    length_set = false;
    activity   = board_millis();
    ui_upload_begin(requested == CHANNEL_GIF);
    return true;
}

/* The old upload stops being valid here, so an interrupted upload falls back to the default */
bool media_upload_length(uint32_t length) {
    if (channel == CHANNEL_NONE || length_set) {
        return false;
    }
    bool ok;
    if (channel == CHANNEL_IMAGE) {
        ok = length == IMAGE_BYTES && erase_sector(IMAGE_FLAG);
    } else {
        ok = length && length <= MAX_GIF_BYTES && length <= GIF_END - GIF_START && erase_sector(GIF_FLAG) && erase_sector(GIF_LENGTH) &&
             program_word(GIF_LENGTH, length);
    }
    if (!ok) {
        media_upload_cancel();
        return false;
    }
    total        = length;
    index_next   = 0;
    erased_until = channel == CHANNEL_IMAGE ? IMAGE_START : GIF_START;
    length_set   = true;
    activity     = board_millis();
    return true;
}

bool media_on_packet(const uint8_t *packet, uint8_t length) {
    const uint8_t  data_length = (uint8_t)(length - PACKET_HEADER - PACKET_CRC);
    const uint8_t *crc         = packet + length - PACKET_CRC;
    const uint32_t expected    = ((uint32_t)crc[0] << 24) | ((uint32_t)crc[1] << 16) | ((uint32_t)crc[2] << 8) | crc[3];
    const uint32_t start       = (channel == CHANNEL_IMAGE ? IMAGE_START : GIF_START) + index_next * PACKET_DATA;

    if (upload_crc(packet, length - PACKET_CRC) != expected) {
        media_upload_cancel();
        return false;
    }

    for (uint32_t i = 0; i < data_length; i += 4) {
        const uint32_t address = start + i;
        while (address >= erased_until) {
            if (!erase_sector(erased_until)) {
                media_upload_cancel();
                return false;
            }
            erased_until += SECTOR_SIZE;
        }
        uint8_t bytes[4] = {0, 0, 0, 0};
        for (uint32_t j = 0; j < 4 && i + j < data_length; j++) {
            bytes[j] = packet[PACKET_HEADER + i + j];
        }
        const uint32_t word = bytes[0] | ((uint32_t)bytes[1] << 8) | ((uint32_t)bytes[2] << 16) | ((uint32_t)bytes[3] << 24);
        if (!program_word(address, word)) {
            media_upload_cancel();
            return false;
        }
    }

    index_next++;
    activity = board_millis();

    const uint32_t done = index_next * PACKET_DATA;
    ui_upload_progress((uint8_t)((done < total ? done : total) * 100u / total));
    return true;
}

bool media_upload_end(void) {
    if (!media_upload_receiving() || index_next * PACKET_DATA < total) {
        media_upload_cancel();
        return false;
    }
    const bool ok = program_word(channel == CHANNEL_IMAGE ? IMAGE_FLAG : GIF_FLAG, FLAG_SET);
    media_upload_cancel();
    return ok;
}

bool media_delete(uint8_t command) {
    const bool ok = erase_sector(command == COMMAND_DELETE_IMAGE ? IMAGE_FLAG : GIF_FLAG);
    ui_media_changed();
    return ok;
}

void media_task(void) {
    if (channel != CHANNEL_NONE && board_millis() - activity >= UPLOAD_TIMEOUT_MS) {
        media_upload_cancel();
    }
}

void media_page_enter(bool show_gif) {
    page_gif   = show_gif;
    page_drawn = false;
    gif_ready  = false;
}

/* The page swaps in place with no clear, so only what the new picture does not cover is erased */
static void clear_margins(uint8_t x, uint8_t y, uint8_t width, uint8_t height) {
    const uint8_t right  = x + width;
    const uint8_t bottom = y + height;

    if (y) {
        draw_fill(0, 0, LCD_WIDTH - 1, y - 1, 0);
    }
    if (bottom < LCD_HEIGHT) {
        draw_fill(0, bottom, LCD_WIDTH - 1, LCD_HEIGHT - 1, 0);
    }
    if (x) {
        draw_fill(0, y, x - 1, bottom - 1, 0);
    }
    if (right < LCD_WIDTH) {
        draw_fill(right, y, LCD_WIDTH - 1, bottom - 1, 0);
    }
}

static void draw_message(const char *text) {
    page_changed = true;
    draw_fill(0, 0, LCD_WIDTH - 1, LCD_HEIGHT - 1, 0);
    draw_text((LCD_WIDTH - draw_text_width(&font_small, text)) / 2, MESSAGE_Y, &font_small, text, 0xFFFF, 0);
}

static void draw_canvas(void) {
    const uint8_t width  = gif_width();
    const uint8_t height = gif_height();
    const uint8_t x      = (LCD_WIDTH - width) / 2;
    const uint8_t y      = (LCD_HEIGHT - height) / 2;

    page_changed = true;
    lcd_send_rect(x, y, x + width - 1, y + height - 1, gif_canvas());

    if (margins_stale) {
        margins_stale = false;
        clear_margins(x, y, width, height);
    }
}

static bool open_gif(void) {
    if (extflash_read_word(GIF_FLAG) == FLAG_SET) {
        const uint32_t length = extflash_read_word(GIF_LENGTH);
        if (length && length <= GIF_END - GIF_START && gif_open(GIF_START, length)) {
            return true;
        }
    }
    return gif_open(DEFAULT_GIF, DEFAULT_GIF_LENGTH);
}

bool media_page_take_drawn(void) {
    const bool changed = page_changed;
    page_changed       = false;
    return changed;
}

void media_page_redraw(void) {
    if (page_gif && gif_ready) {
        margins_stale = true;
        draw_canvas();
    } else {
        page_drawn = false;
    }
}

void media_page_task(bool playing) {
    if (!page_gif) {
        if (!page_drawn) {
            page_drawn = true;
            const uint32_t address = extflash_read_word(IMAGE_FLAG) == FLAG_SET ? IMAGE_START : DEFAULT_IMAGE;
            const uint8_t  margin  = (LCD_WIDTH - IMAGE_SIDE) / 2;
            extflash_draw_raw(address, margin, margin, IMAGE_SIDE, IMAGE_SIDE);
            clear_margins(margin, margin, IMAGE_SIDE, IMAGE_SIDE);
            page_changed = true;
        }
        return;
    }

    if (page_drawn && !gif_ready) {
        return;
    }
    if (!page_drawn) {
        page_drawn = true;
        gif_ready  = open_gif();
        if (!gif_ready) {
            draw_message("BAD GIF");
            return;
        }
        frame_delay   = 0;
        frame_timer   = board_millis();
        margins_stale = true;
    }
    if ((!playing && frame_delay) || board_millis() - frame_timer < frame_delay) {
        return;
    }

    uint32_t delay;
    frame_timer = board_millis();
    if (!gif_next_frame(&delay)) {
        gif_ready = false;
        draw_message("BAD GIF");
        return;
    }
    frame_delay = delay;
    draw_canvas();
}

bool media_page_gif_rect(uint8_t *x, uint8_t *y, uint8_t *width, uint8_t *height) {
    if (!page_gif || !gif_ready) {
        return false;
    }
    *width  = gif_width();
    *height = gif_height();
    *x      = (LCD_WIDTH - *width) / 2;
    *y      = (LCD_HEIGHT - *height) / 2;
    return true;
}
