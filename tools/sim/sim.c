/*
 * Runs the firmware's UI on a PC and saves what it draws. The UI, drawing, GIF and flash
 * reading code is the real firmware's. Only the LCD and the clock are replaced here, and
 * the flash dumps are mapped at their real addresses.
 *
 * usage: sim <internal flash dump> <external flash dump> <output dir> < script
 */
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "board.h"
#include "lcd.h"
#include "panel.h"
#include "ui.h"

#define INTERNAL_FLASH 0x08000000u
#define INTERNAL_FLASH_SIZE 0x100000u
#define EXTERNAL_FLASH 0x08400000u
#define EXTERNAL_FLASH_SIZE 0x400000u
#define BUILD_FIELD_SIZE 10
#define NAME_SIZE 20
#define QMK_VERSION_SIZE 18

static uint16_t framebuffer[LCD_HEIGHT][LCD_WIDTH];
static uint32_t now;
static bool     powered = true;
static bool     backlight = true;

static uint8_t window_x0, window_y0, window_x1, window_y1;
static int     cursor_x, cursor_y;

void board_init(void) {
}

uint32_t board_millis(void) {
    return now;
}

void board_delay_ms(uint32_t ms) {
    now += ms;
}

void board_reset(void) {
    exit(0);
}

void lcd_init(void) {
}

void lcd_set_power(bool on) {
    powered = on;
}

void lcd_set_backlight(bool on) {
    backlight = on;
}

void lcd_set_brightness(uint8_t percent) {
    (void)percent;
}

void panel_window(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1) {
    window_x0 = x0;
    window_y0 = y0;
    window_x1 = x1;
    window_y1 = y1;
    cursor_x  = x0;
    cursor_y  = y0;
}

/* Same order as the panel: left to right, then top to bottom */
void panel_pixel(uint16_t color) {
    if (cursor_x >= 0 && cursor_x < LCD_WIDTH && cursor_y < LCD_HEIGHT) {
        framebuffer[cursor_y][cursor_x] = color;
    }
    if (++cursor_x > window_x1) {
        cursor_x = window_x0;
        cursor_y++;
    }
}

void panel_write(const uint8_t *bytes, uint16_t len) {
    for (uint16_t i = 0; i + 1 < len; i += 2) {
        panel_pixel((uint16_t)(bytes[i] << 8 | bytes[i + 1]));
    }
}

void panel_end(void) {
}

void panel_send_rect(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, const uint8_t *pixels) {
    panel_window(x0, y0, x1, y1);
    panel_write(pixels, (uint16_t)((x1 - x0 + 1u) * (y1 - y0 + 1u) * 2u));
}

void lcd_wait_idle(void) {
}

static void map_flash(const char *path, uint32_t address, uint32_t size) {
    void *memory = mmap((void *)(uintptr_t)address, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE, -1, 0);
    if (memory != (void *)(uintptr_t)address) {
        fprintf(stderr, "cannot map %08x: %s\n", address, strerror(errno));
        exit(1);
    }
    memset(memory, 0xFF, size);

    const int file = open(path, O_RDONLY);
    if (file < 0 || read(file, memory, size) < 0) {
        fprintf(stderr, "cannot read %s: %s\n", path, strerror(errno));
        exit(1);
    }
    close(file);
}

static void run(uint32_t ms) {
    for (uint32_t i = 0; i < ms; i++) {
        now++;
        ui_task();
    }
}

static void save(const char *dir, const char *name) {
    char path[512];
    snprintf(path, sizeof(path), "%s/%s.ppm", dir, name);

    FILE *file = fopen(path, "wb");
    if (!file) {
        fprintf(stderr, "cannot write %s: %s\n", path, strerror(errno));
        exit(1);
    }
    fprintf(file, "P6\n%d %d\n255\n", LCD_WIDTH, LCD_HEIGHT);
    for (int y = 0; y < LCD_HEIGHT; y++) {
        for (int x = 0; x < LCD_WIDTH; x++) {
            const uint16_t color = powered && backlight ? framebuffer[y][x] : 0;
            const uint8_t  r     = (uint8_t)(((color >> 11) * 255 + 15) / 31);
            const uint8_t  g     = (uint8_t)((((color >> 5) & 0x3F) * 255 + 31) / 63);
            const uint8_t  b     = (uint8_t)(((color & 0x1F) * 255 + 15) / 31);
            const uint8_t  pixel[3] = {r, g, b};
            fwrite(pixel, 1, 3, file);
        }
    }
    fclose(file);
}

static void command(const char *dir, char *line) {
    char name[64] = {0}, text[64] = {0}, other[64] = {0};
    int  a, b, c, d, e, f;

    if (line[0] == '#' || line[0] == '\n') {
        return;
    }
    if (sscanf(line, "wait %d", &a) == 1) {
        run((uint32_t)a);
    } else if (sscanf(line, "shot %63s", name) == 1) {
        save(dir, name);
    } else if (sscanf(line, "mode %d", &a) == 1) {
        ui_on_mode((uint8_t)a);
    } else if (sscanf(line, "peek %d", &a) == 1) {
        ui_on_peek((uint8_t)a);
    } else if (sscanf(line, "accent %d", &a) == 1) {
        ui_on_accent((uint8_t)a);
    } else if (sscanf(line, "units %d", &a) == 1) {
        ui_on_units((uint8_t)a);
    } else if (sscanf(line, "face %d", &a) == 1) {
        ui_on_clock_face((uint8_t)a);
    } else if (sscanf(line, "media %d", &a) == 1) {
        ui_on_media((uint8_t)a);
    } else if (sscanf(line, "wpm %d", &a) == 1) {
        ui_on_wpm((uint16_t)a);
    } else if (sscanf(line, "tap %d %d", &a, &b) == 2) {
        ui_on_key_tap((uint8_t)a, (uint8_t)b);
    } else if (sscanf(line, "keys %d %i", &a, &b) == 2) {
        ui_on_keys((uint8_t)a, (uint16_t)b);
    } else if (sscanf(line, "status %d %d %d %d", &a, &b, &c, &d) == 4) {
        ui_on_status((uint8_t)a, (uint8_t)b, c != 0, (uint8_t)d);
    } else if (sscanf(line, "extra %d", &a) == 1) {
        ui_on_status_extra((uint8_t)a);
    } else if (sscanf(line, "rgb %d %d %d %d %d %d", &a, &b, &c, &d, &e, &f) == 6) {
        ui_on_rgb(a != 0, (uint8_t)b, (uint8_t)c, (uint8_t)d, (uint8_t)e, (uint8_t)f);
    } else if (sscanf(line, "rgbname %63[^\n]", text) == 1) {
        char padded[NAME_SIZE] = {0};
        strncpy(padded, text, NAME_SIZE - 1);
        ui_on_rgb_name(padded);
    } else if (sscanf(line, "build %63s %63s", text, other) == 2) {
        char build[BUILD_FIELD_SIZE * 2] = {0};
        strncpy(build, text, BUILD_FIELD_SIZE);
        strncpy(build + BUILD_FIELD_SIZE, other, BUILD_FIELD_SIZE);
        ui_on_build(build);
    } else if (sscanf(line, "versions %63s %d", text, &a) == 2) {
        char qmk[QMK_VERSION_SIZE] = {0};
        strncpy(qmk, text, QMK_VERSION_SIZE - 1);
        ui_on_versions(qmk, (uint16_t)a);
    } else if (sscanf(line, "time %d %d %d %d %d %d", &a, &b, &c, &d, &e, &f) == 6) {
        ui_on_time((uint8_t)a, (uint8_t)b, (uint8_t)c, (uint8_t)d, (uint8_t)e, (uint8_t)f);
    } else if (sscanf(line, "system %d %d %d", &a, &b, &c) == 3) {
        ui_on_system((uint8_t)a, (uint8_t)b, (uint16_t)c);
    } else if (sscanf(line, "weather %d %d %d %d", &a, &b, &c, &d) == 4) {
        ui_on_weather((uint8_t)a, (uint8_t)b, (uint8_t)c, (uint8_t)d);
    } else if (sscanf(line, "upload %d %d", &a, &b) == 2) {
        ui_upload_begin(a != 0);
        ui_upload_progress((uint8_t)b);
    } else {
        fprintf(stderr, "unknown command: %s", line);
        exit(1);
    }
}

int main(int argc, char **argv) {
    if (argc != 4) {
        fprintf(stderr, "usage: %s <internal flash dump> <external flash dump> <output dir> < script\n", argv[0]);
        return 1;
    }
    map_flash(argv[1], INTERNAL_FLASH, INTERNAL_FLASH_SIZE);
    map_flash(argv[2], EXTERNAL_FLASH, EXTERNAL_FLASH_SIZE);

    ui_init();

    char line[256];
    while (fgets(line, sizeof(line), stdin)) {
        command(argv[3], line);
    }
    return 0;
}
