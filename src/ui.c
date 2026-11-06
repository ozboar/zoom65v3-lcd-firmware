#include "assets.h"
#include "board.h"
#include "draw.h"
#include "extflash.h"
#include "keyvis.h"
#include "lcd.h"
#include "media.h"
#include "ui.h"

#define COLOR_BLACK RGB565(0, 0, 0)
#define COLOR_WHITE RGB565(255, 255, 255)
#define COLOR_GRID RGB565(60, 60, 60)
#define COLOR_LOW RGB565(230, 60, 60)
#define COLOR_DIM RGB565(150, 150, 150)
#define COLOR_DIM_ON_ACCENT RGB565(90, 90, 90)

#define LAST_X (LCD_WIDTH - 1)
#define LAST_Y (LCD_HEIGHT - 1)

/* Positions from tools/generate_bongo.py */
#define BONGO_HEIGHT 92
#define BONGO_LEFT_X 69
#define BONGO_LEFT_Y 36
#define BONGO_RIGHT_X 9
#define BONGO_RIGHT_Y 14
#define BONGO_TAP_MS 100
#define BONGO_SPAM_MS 60
#define BONGO_EYE_RIGHT_X 40
#define BONGO_EYE_RIGHT_Y 30
#define BONGO_EYE_LEFT_X 71
#define BONGO_EYE_LEFT_Y 40
#define BONGO_EYE_LID_OFFSET 2
#define BONGO_EYE_LID_HEIGHT 2
#define BONGO_BLINK_CLOSED_MS 110
#define BONGO_SPAM_ROW 0
#define BONGO_SPAM_COL 13
#define BONGO_INTRO_TAPS 4
#define BONGO_INTRO_STEP_MS 160
#define CAT_KEY_FADE_MS 600
#define CAT_KEY_FADE_STEP_MS 30
#define CAT_KEY_LEVEL_MAX 255
/* Red paw shading stays a darker shade of the accent */
#define ACCENT_SHADE_PERCENT 55
#define FIRST_MODE_WAIT_MS 500
#define WPM_MARGIN 4
#define WPM_LABEL_GAP 2
#define BONGO_TIME_LEFT 62
#define BONGO_TIME_RIGHT 111
#define BONGO_TIME_Y 103
#define WPM_UPDATE_MS 250

/* One column per sample, about two minutes across */
#define GRAPH_LEFT 4
#define GRAPH_RIGHT 113
#define GRAPH_TOP 30
#define GRAPH_BOTTOM 113
#define GRAPH_WIDTH (GRAPH_RIGHT - GRAPH_LEFT + 1)
#define GRAPH_HEIGHT (GRAPH_BOTTOM - GRAPH_TOP + 1)
#define GRAPH_SAMPLE_MS 1000
#define GRAPH_GRID_WPM 25
#define GRAPH_MIN_SCALE 50
#define GRAPH_HEADER_Y 4
#define GRAPH_PEAK_Y 15

/* System mode: three rows, with values from zoom-sync */
#define SYSTEM_ROWS 3
#define SYSTEM_TOP(row) ((uint8_t)((row) * LCD_HEIGHT / SYSTEM_ROWS))
#define SYSTEM_MARGIN 4
#define SYSTEM_LABEL_Y 3
#define SYSTEM_VALUE_Y 15
#define SYSTEM_BAR_LEFT 68
#define SYSTEM_BAR_RIGHT 113
#define SYSTEM_BAR_Y 19
#define SYSTEM_BAR_HEIGHT 12
#define SYSTEM_BAR_FULL 100
#define SYSTEM_DOT_SIZE 3
#define SYSTEM_DOT_GAP 2
#define SYSTEM_STALE_MS 30000

/* Weather mode: a doubled stock icon above the temperatures */
#define WEATHER_ICON_AREA_TOP 4
#define WEATHER_ICON_AREA_HEIGHT 62
#define WEATHER_TEMP_Y 70
#define WEATHER_RANGE_Y 98
#define WEATHER_DEGREE_SIZE 4
#define WEATHER_DEGREE_GAP 2

/* Set bits show Fahrenheit. zoom-sync is expected to send Celsius */
#define UNITS_SYSTEM_F (1u << 0)
#define UNITS_WEATHER_F (1u << 1)

/* Page menu: shown when switching pages, then the page opens */
#define MENU_SHOW_MS 800
#define MENU_TOP(row) ((uint8_t)((row) * LCD_HEIGHT / MODE_COUNT))
#define MENU_BAR_MARGIN 3

/* Dots shown briefly when a page's setting changes */
#define PILL_SHOW_MS 800
#define PILL_Y 100
#define PILL_HEIGHT 11
#define PILL_PADDING 6
#define PILL_DOT_SIZE 5
#define PILL_DOT_GAP 4

/* Image and GIF page */
#define MEDIA_SETTING_IMAGE 0
#define MEDIA_SETTING_REACTIVE 2
#define MEDIA_SETTING_COUNT 3
#define MEDIA_REACTIVE_MS 400

/* Upload progress */
#define UPLOAD_TEXT_Y 44
#define UPLOAD_BAR_LEFT 14
#define UPLOAD_BAR_RIGHT 103
#define UPLOAD_BAR_Y 60
#define UPLOAD_BAR_HEIGHT 6

#define MENU_MARKER_X 6
#define MENU_TEXT_X 14

/* Clock mode */
enum { CLOCK_FACE_DETAILED, CLOCK_FACE_SIMPLE, CLOCK_FACE_DIGITAL, CLOCK_FACE_BINARY, CLOCK_FACE_COUNT };
#define CLOCK_CENTER 59
#define CLOCK_RADIUS 56
#define CLOCK_SIMPLE_TICK 48
#define CLOCK_SIMPLE_HOUR 26
#define CLOCK_SIMPLE_MINUTE 42
#define CLOCK_NUMERAL_RADIUS 44
#define CLOCK_DETAILED_HOUR 20
#define CLOCK_DETAILED_MINUTE 31
#define CLOCK_DETAILED_SECOND 35
#define CLOCK_DATE_Y 72
#define CLOCK_COLON_WIDTH 10
#define CLOCK_COLON_SIZE 3
#define CLOCK_DIGIT_GAP 3
#define CLOCK_DIGITAL_Y 46
#define CLOCK_DIGITAL_DATE_Y 28
#define CLOCK_BAR_LEFT 14
#define CLOCK_BAR_RIGHT 103
#define CLOCK_BAR_Y 76
#define CLOCK_CELL 13
#define CLOCK_CELL_GAP 2
#define CLOCK_PAIR_GAP 4
#define CLOCK_BINARY_LEFT 10
#define CLOCK_BINARY_TOP 23
#define CLOCK_BINARY_LABEL_Y 87

#define RIGHT_EDGE 114
#define STATUS_MARGIN 4
#define STATUS_HEADER_Y 3
#define STATUS_ICON_WIDTH 20
#define STATUS_ICON_HEIGHT 11
#define STATUS_DIVIDER_Y 16
#define STATUS_TILES_Y 20
#define STATUS_TILE_WIDTH 54
#define STATUS_TILE_HEIGHT 26
#define STATUS_TILE_GAP 2
#define STATUS_KEYS_Y 78
#define STATUS_UPTIME_X 64
#define STATUS_LEFT_VALUE_EDGE 59
#define STATUS_QMK_Y 92
#define STATUS_QMK_VERSION_SIZE 18
#define STATUS_BUILD_Y 106
#define STATUS_BUILD_FIELD_SIZE 10
#define STATUS_BATTERY_LOW 20
#define STATUS_FLAG_CHARGING (1u << 0)
#define STATUS_FLAG_FULL (1u << 1)
#define STATUS_FLAG_NKRO (1u << 2)

/* RGB mode: effect details above a swatch of the current color */
#define RGB_MARGIN 4
#define RGB_HEADER_Y 3
#define RGB_NAME_Y 16
#define RGB_DIVIDER_Y 29
#define RGB_ROWS_Y 32
#define RGB_ROW_HEIGHT 13
#define RGB_BAR_LEFT 34
#define RGB_BAR_RIGHT 84
#define RGB_BAR_HEIGHT 7
#define RGB_SWATCH_Y 100
#define RGB_SWATCH_BOTTOM 113
#define RGB_NAME_SIZE 20

#define KEY_UNITS 64
#define KEY_CENTER_UNITS (KEY_UNITS / 2)

enum { STATUS_ROW_CONNECTION, STATUS_ROW_LAYER, STATUS_ROW_CAPS, STATUS_ROW_NKRO, STATUS_ROW_KEYS, STATUS_ROW_UPTIME, STATUS_ROW_QMK, STATUS_ROW_BUILD, STATUS_ROW_COUNT };
enum { MODE_CLOCK, MODE_MEDIA, MODE_BONGO, MODE_GRAPH, MODE_SYSTEM, MODE_WEATHER, MODE_RGB, MODE_STATUS, MODE_COUNT };
enum { PAW_NONE, PAW_LEFT, PAW_RIGHT };

static uint8_t  mode;
static uint8_t  units;

static bool     clock_valid;
static uint8_t  clock_year;
static uint8_t  clock_month;
static uint8_t  clock_day;
static uint32_t clock_base_seconds;
static uint32_t clock_base_millis;
static uint8_t  clock_face;
static uint32_t clock_drawn_seconds;
static uint8_t  clock_drawn_hour;
static uint8_t  clock_drawn_minute;
static uint8_t  clock_drawn_second;
static uint8_t  clock_drawn_digits[6];
static bool     clock_dial_drawn;
static bool     clock_empty_drawn;
static bool     menu_active;
static uint8_t  menu_drawn;
static uint32_t menu_timer;
static bool     waiting_for_mode = true;
static uint32_t start_time;
static bool     powered = true;
static bool     iso_layout;

static uint8_t  paw_drawn;
static uint8_t  paw_target;
static uint8_t  paw_next = PAW_LEFT;
static uint32_t paw_timer;
static uint8_t  intro_taps;
static uint32_t intro_timer;
static uint16_t cat_keys_drawn[KEY_ROWS];
static uint8_t  cat_key_level[KEY_ROWS][KEY_COLS];
static uint32_t cat_fade_timer;
static uint8_t  accent;

static const uint16_t accent_colors[] = {
    RGB565(255, 150, 160), /* Pink, as drawn */
    RGB565(90, 190, 255),  /* Blue */
    RGB565(100, 230, 140), /* Green */
    RGB565(255, 210, 80),  /* Yellow */
    RGB565(185, 135, 255), /* Purple */
    RGB565(255, 130, 70),  /* Orange */
    RGB565(255, 255, 255), /* White */
};
#define ACCENT_COUNT (sizeof(accent_colors) / sizeof(accent_colors[0]))

static uint16_t wpm;
static int32_t  wpm_drawn;
static uint32_t wpm_timer;
static int32_t  bongo_time_drawn;
static bool     blink_closed;
static bool     pill_active;
static uint8_t  pill_count;
static uint8_t  pill_selected;
static uint32_t pill_timer;
static bool     media_show_gif;
static uint8_t  media_setting;
static bool     media_reactive;
static uint32_t media_key_timer;
static bool     upload_gif;
static uint8_t  upload_percent;
static uint8_t  blink_step;
static uint32_t blink_timer;

static uint8_t status_device;
static uint8_t status_layer;
static bool    status_caps;
static uint8_t status_battery;
static int32_t status_drawn[STATUS_ROW_COUNT];
static int32_t status_header_drawn;
static uint8_t status_flags;
static bool    status_extra_received;
static bool    status_build_received;
static bool    status_qmk_received;
static char    status_qmk_version[STATUS_QMK_VERSION_SIZE];
static uint16_t status_via_version;
static char    status_build_date[STATUS_BUILD_FIELD_SIZE + 1];
static char    status_build_hash[STATUS_BUILD_FIELD_SIZE + 1];
static uint32_t status_key_count;

static bool    rgb_received;
static bool    rgb_enabled;
static uint8_t rgb_mode;
static uint8_t rgb_hue;
static uint8_t rgb_saturation;
static uint8_t rgb_value;
static uint8_t rgb_speed;
static char    rgb_name[RGB_NAME_SIZE];
static bool    rgb_dirty;

static uint16_t history[GRAPH_WIDTH];
static uint8_t  history_head;
static uint32_t history_timer;
static uint16_t peak;
static int32_t  peak_drawn;

static uint8_t  system_cpu;
static uint8_t  system_gpu;
static uint16_t system_download;
static bool     system_received;
static uint32_t system_timer;
static int32_t  system_drawn[SYSTEM_ROWS];

static uint8_t weather_icon;
static uint8_t weather_current;
static uint8_t weather_low;
static uint8_t weather_high;
static bool    weather_received;
static bool    weather_dirty;

static uint16_t keys_state[KEY_ROWS];

/* Appends a decimal number and returns the end of the text */
static char *append_number(char *out, uint32_t value) {
    char     digits[10];
    uint8_t  count = 0;
    do {
        digits[count++] = (char)('0' + value % 10);
        value /= 10;
    } while (value);
    while (count) {
        *out++ = digits[--count];
    }
    *out = 0;
    return out;
}

static char *append_text(char *out, const char *text) {
    while (*text) {
        *out++ = *text++;
    }
    *out = 0;
    return out;
}

static void clear_screen(void) {
    draw_fill(0, 0, LAST_X, LAST_Y, COLOR_BLACK);
}

static void draw_right_aligned(uint8_t y, const char *text) {
    const uint8_t width = draw_text_width(&font_small, text);
    draw_text(RIGHT_EDGE - width, y, &font_small, text, COLOR_WHITE, COLOR_BLACK);
}

/* Status mode: a small battery in a header, above a list of readings */

static const char *device_name(uint8_t device) {
    static const char *const names[] = {"USB", "BT1", "BT2", "BT3", "BT4", "BT5", "2.4G"};
    return device < sizeof(names) / sizeof(names[0]) ? names[device] : "?";
}

static char *append_two_digits(char *out, uint32_t value) {
    if (value < 10) {
        out = append_text(out, "0");
    }
    return append_number(out, value);
}

static const char *status_charge_text(void) {
    if (!status_extra_received || !(status_flags & STATUS_FLAG_CHARGING)) {
        return "";
    }
    return status_flags & STATUS_FLAG_FULL ? "FULL" : "CHG";
}

static void status_draw_header(void) {
    char           text[6];
    const uint32_t percent = status_battery > 100 ? 100 : status_battery;
    const uint8_t  right   = STATUS_MARGIN + STATUS_ICON_WIDTH - 1;
    const uint8_t  bottom  = STATUS_HEADER_Y + STATUS_ICON_HEIGHT - 1;
    const uint8_t  width   = STATUS_ICON_WIDTH - 4;
    const uint8_t  fill    = (uint8_t)((uint32_t)width * percent / 100);

    /* A filled band, with the battery and text in black on it */
    draw_fill(0, 0, LAST_X, STATUS_DIVIDER_Y, accent_colors[accent]);
    draw_outline(STATUS_MARGIN, STATUS_HEADER_Y, right, bottom, COLOR_BLACK);
    draw_fill(right + 1, STATUS_HEADER_Y + 3, right + 2, bottom - 3, COLOR_BLACK);
    if (fill) {
        draw_fill(STATUS_MARGIN + 2, STATUS_HEADER_Y + 2, STATUS_MARGIN + 1 + fill, bottom - 2, percent < STATUS_BATTERY_LOW ? COLOR_LOW : COLOR_BLACK);
    }

    append_text(append_number(text, status_battery), "%");
    draw_text(right + 5, STATUS_HEADER_Y, &font_small, text, COLOR_BLACK, accent_colors[accent]);

    const char *charge = status_charge_text();
    draw_text(RIGHT_EDGE - draw_text_width(&font_small, charge), STATUS_HEADER_Y, &font_small, charge, COLOR_BLACK, accent_colors[accent]);
}

static uint32_t status_uptime_seconds(void) {
    return board_millis() / 1000;
}

static int32_t status_value(uint8_t row) {
    switch (row) {
        case STATUS_ROW_CONNECTION:
            return status_device;
        case STATUS_ROW_LAYER:
            return status_layer;
        case STATUS_ROW_CAPS:
            return status_caps;
        case STATUS_ROW_NKRO:
            return status_extra_received ? (status_flags & STATUS_FLAG_NKRO) != 0 : -2;
        case STATUS_ROW_KEYS:
            return (int32_t)status_key_count;
        case STATUS_ROW_BUILD:
            return status_build_received;
        case STATUS_ROW_QMK:
            return status_qmk_received;
        default:
            return (int32_t)status_uptime_seconds();
    }
}

/* State readings are tiles that fill in when active */
static void status_draw_tile(uint8_t index, const char *label, const char *value, bool active) {
    const uint8_t  left   = STATUS_MARGIN + (index % 2) * (STATUS_TILE_WIDTH + STATUS_TILE_GAP);
    const uint8_t  top    = STATUS_TILES_Y + (index / 2) * (STATUS_TILE_HEIGHT + STATUS_TILE_GAP);
    const uint8_t  right  = left + STATUS_TILE_WIDTH - 1;
    const uint8_t  bottom = top + STATUS_TILE_HEIGHT - 1;
    const uint16_t bg     = active ? accent_colors[accent] : COLOR_BLACK;

    draw_fill(left, top, right, bottom, bg);
    draw_outline(left, top, right, bottom, active ? bg : COLOR_GRID);
    draw_text(left + (STATUS_TILE_WIDTH - draw_text_width(&font_small, label)) / 2, top + 2, &font_small, label, active ? COLOR_DIM_ON_ACCENT : COLOR_DIM, bg);
    draw_text(left + (STATUS_TILE_WIDTH - draw_text_width(&font_small, value)) / 2, top + 13, &font_small, value, active ? COLOR_BLACK : COLOR_WHITE, bg);
}

/* Two readings sharing a line, each half redrawn separately */
static void status_draw_left(uint8_t y, const char *label, const char *value) {
    draw_fill(0, y, STATUS_UPTIME_X - 1, y + font_small.height - 1, COLOR_BLACK);
    draw_text(STATUS_MARGIN, y, &font_small, label, COLOR_DIM, COLOR_BLACK);
    draw_text(STATUS_LEFT_VALUE_EDGE - draw_text_width(&font_small, value), y, &font_small, value, COLOR_WHITE, COLOR_BLACK);
}

static void status_draw_right(uint8_t y, const char *label, const char *value) {
    draw_fill(STATUS_UPTIME_X, y, LAST_X, y + font_small.height - 1, COLOR_BLACK);
    draw_text(STATUS_UPTIME_X, y, &font_small, label, COLOR_DIM, COLOR_BLACK);
    draw_right_aligned(y, value);
}

/* Counters are plain lines below the tiles */
static void status_draw_line(uint8_t y, const char *label, const char *value) {
    draw_fill(0, y, LAST_X, y + font_small.height - 1, COLOR_BLACK);
    draw_text(STATUS_MARGIN, y, &font_small, label, COLOR_DIM, COLOR_BLACK);
    draw_right_aligned(y, value);
}

static void status_draw_row(uint8_t row) {
    const uint32_t seconds = status_uptime_seconds();
    char           text[16];

    text[0] = 0;
    switch (row) {
        case STATUS_ROW_CONNECTION:
            status_draw_tile(0, "CONN", device_name(status_device), false);
            break;
        case STATUS_ROW_LAYER:
            append_number(text, status_layer);
            status_draw_tile(1, "LAYER", text, status_layer != 0);
            break;
        case STATUS_ROW_CAPS:
            status_draw_tile(2, "CAPS", status_caps ? "ON" : "OFF", status_caps);
            break;
        case STATUS_ROW_NKRO:
            status_draw_tile(3, "NKRO", !status_extra_received ? "-" : status_flags & STATUS_FLAG_NKRO ? "ON" : "OFF", status_extra_received && (status_flags & STATUS_FLAG_NKRO));
            break;
        case STATUS_ROW_KEYS:
            append_number(text, status_key_count);
            status_draw_left(STATUS_KEYS_Y, "KEYS", text);
            break;
        case STATUS_ROW_QMK:
            status_draw_left(STATUS_QMK_Y, "QMK", status_qmk_version);
            if (status_via_version) {
                append_number(text, status_via_version);
            } else {
                append_text(text, "-");
            }
            status_draw_right(STATUS_QMK_Y, "VIA", text);
            break;
        case STATUS_ROW_BUILD: {
            /* The date arrives as YYYY-MM-DD and is shown as MM/DD/YY */
            const char *iso    = status_build_date;
            const char  date[] = {iso[5], iso[6], '/', iso[8], iso[9], '/', iso[2], iso[3], 0};
            char        build[STATUS_BUILD_FIELD_SIZE * 2 + 3];
            build[0] = 0;
            append_text(append_text(append_text(build, date), "    "), status_build_hash);
            status_draw_line(STATUS_BUILD_Y, "BUILD", build);
            break;
        }
        default:
            append_two_digits(append_text(append_two_digits(append_text(append_number(text, seconds / 3600), ":"), seconds / 60 % 60), ":"), seconds % 60);
            status_draw_right(STATUS_KEYS_Y, "UP", text);
            break;
    }
}

static void status_enter(void) {
    status_header_drawn = -1;
    for (uint8_t row = 0; row < STATUS_ROW_COUNT; row++) {
        status_drawn[row] = -1;
    }
}

static void status_task(void) {
    const int32_t header = status_battery | (status_extra_received ? (status_flags & (STATUS_FLAG_CHARGING | STATUS_FLAG_FULL)) << 8 : 0);
    if (status_header_drawn != header) {
        status_header_drawn = header;
        status_draw_header();
    }

    for (uint8_t row = 0; row < STATUS_ROW_COUNT; row++) {
        const int32_t value = status_value(row);
        if (status_drawn[row] != value) {
            status_drawn[row] = value;
            status_draw_row(row);
        }
    }
}

static void centered_small_text(uint8_t y, const char *text) {
    draw_text((LCD_WIDTH - draw_text_width(&font_small, text)) / 2, y, &font_small, text, COLOR_WHITE, COLOR_BLACK);
}

/* RGB mode: the keyboard's current effect and color settings */

static uint16_t hsv_to_color(uint8_t hue, uint8_t saturation, uint8_t value) {
    const uint32_t region = hue / 43u;
    const uint32_t rem    = (hue - region * 43u) * 6u;
    const uint32_t p      = value * (255u - saturation) / 255u;
    const uint32_t q      = value * (255u - saturation * rem / 255u) / 255u;
    const uint32_t t      = value * (255u - saturation * (255u - rem) / 255u) / 255u;

    switch (region) {
        case 0:
            return RGB565(value, t, p);
        case 1:
            return RGB565(q, value, p);
        case 2:
            return RGB565(p, value, t);
        case 3:
            return RGB565(p, q, value);
        case 4:
            return RGB565(t, p, value);
        default:
            return RGB565(value, p, q);
    }
}

static void rgb_draw_row(uint8_t index, const char *label, uint32_t level, const char *suffix, uint32_t shown, uint16_t color) {
    char          text[10];
    const uint8_t y     = RGB_ROWS_Y + index * RGB_ROW_HEIGHT;
    const uint8_t bar_y = y + (RGB_ROW_HEIGHT - RGB_BAR_HEIGHT) / 2;
    const uint8_t fill  = (uint8_t)((uint32_t)(RGB_BAR_RIGHT - RGB_BAR_LEFT - 1) * level / 255);

    draw_text(RGB_MARGIN, y + 1, &font_small, label, COLOR_WHITE, COLOR_BLACK);
    draw_outline(RGB_BAR_LEFT, bar_y, RGB_BAR_RIGHT, bar_y + RGB_BAR_HEIGHT - 1, COLOR_GRID);
    if (fill) {
        draw_fill(RGB_BAR_LEFT + 1, bar_y + 1, RGB_BAR_LEFT + fill, bar_y + RGB_BAR_HEIGHT - 2, color);
    }
    append_text(append_number(text, shown), suffix);
    draw_right_aligned(y + 1, text);
}

static void rgb_render(void) {
    char text[16];

    clear_screen();
    if (!rgb_received) {
        centered_small_text((LCD_HEIGHT - font_small.height) / 2, "NO DATA");
        return;
    }

    draw_text(RGB_MARGIN, RGB_HEADER_Y, &font_small, "RGB", COLOR_WHITE, COLOR_BLACK);
    append_number(append_text(text, "#"), rgb_mode);
    centered_small_text(RGB_HEADER_Y, text);
    draw_right_aligned(RGB_HEADER_Y, rgb_enabled ? "ON" : "OFF");
    centered_small_text(RGB_NAME_Y, rgb_name);
    draw_fill(0, RGB_DIVIDER_Y, LAST_X, RGB_DIVIDER_Y, COLOR_GRID);

    const uint16_t current = hsv_to_color(rgb_hue, rgb_saturation, 255);
    rgb_draw_row(0, "HUE", rgb_hue, "", (uint32_t)rgb_hue * 360 / 255, current);
    rgb_draw_row(1, "SAT", rgb_saturation, "%", (uint32_t)rgb_saturation * 100 / 255, current);
    rgb_draw_row(2, "VAL", rgb_value, "%", (uint32_t)rgb_value * 100 / 255, COLOR_WHITE);
    rgb_draw_row(3, "SPD", rgb_speed, "%", (uint32_t)rgb_speed * 100 / 255, COLOR_WHITE);

    /* Value only limits LED brightness, so the swatch uses it too, and goes black when off */
    draw_fill(RGB_MARGIN, RGB_SWATCH_Y, LAST_X - RGB_MARGIN, RGB_SWATCH_BOTTOM, rgb_enabled ? hsv_to_color(rgb_hue, rgb_saturation, rgb_value) : COLOR_BLACK);
    draw_outline(RGB_MARGIN, RGB_SWATCH_Y, LAST_X - RGB_MARGIN, RGB_SWATCH_BOTTOM, COLOR_GRID);
}

static void rgb_task(void) {
    if (rgb_dirty) {
        rgb_dirty = false;
        rgb_render();
    }
}

/* Bongo mode: a press drops the next paw and releasing every key lifts it, redrawing only the paw patches */

static bool mask_bit(const uint8_t *mask, uint32_t bit) {
    return mask[bit / 8] & (1u << (bit % 8));
}

/* False where a paw, up or down, covers this pixel. The two paw patches never overlap */
static bool cat_pixel_visible(uint8_t x, uint8_t y) {
    if (x >= BONGO_LEFT_X && y >= BONGO_LEFT_Y && x < BONGO_LEFT_X + bongo_left_down.width && y < BONGO_LEFT_Y + bongo_left_down.height) {
        const uint32_t bit = (uint32_t)(y - BONGO_LEFT_Y) * bongo_left_down.width + (x - BONGO_LEFT_X);
        return mask_bit(paw_drawn == PAW_LEFT ? cat_mask_left_down : cat_mask_left_up, bit);
    }
    if (x >= BONGO_RIGHT_X && y >= BONGO_RIGHT_Y && x < BONGO_RIGHT_X + bongo_right_down.width && y < BONGO_RIGHT_Y + bongo_right_down.height) {
        const uint32_t bit = (uint32_t)(y - BONGO_RIGHT_Y) * bongo_right_down.width + (x - BONGO_RIGHT_X);
        return mask_bit(paw_drawn == PAW_RIGHT ? cat_mask_right_down : cat_mask_right_up, bit);
    }
    return true;
}

static uint16_t accent_scaled(uint32_t level, uint32_t max) {
    const uint16_t color = accent_colors[accent];
    const uint32_t r     = (color >> 11) * level / max;
    const uint32_t g     = ((color >> 5) & 0x3F) * level / max;
    const uint32_t b     = (color & 0x1F) * level / max;
    return (uint16_t)((r << 11) | (g << 5) | b);
}

/*
 * Paw art is white, black and a pink or red tint. Tinted colors take the accent at the
 * same brightness: pink has green at over half its red, and red has much less.
 */
static uint16_t accent_map(uint16_t color) {
    const uint32_t r = color >> 11;
    const uint32_t g = ((color >> 5) & 0x3F) >> 1;
    const uint32_t b = color & 0x1F;

    if (!r || r < g + 3 || r < b + 3) {
        return color;
    }
    if (g * 100 / r < 45) {
        return accent_scaled(r * ACCENT_SHADE_PERCENT, 31 * 100);
    }
    return accent_scaled(r, 31);
}

static uint16_t cat_key_color(uint8_t level) {
    return accent_scaled(level, CAT_KEY_LEVEL_MAX);
}

static void cat_key_draw(uint8_t row, uint8_t col) {
    const uint16_t index = row * KEY_COLS + col;
    const uint16_t color = cat_key_color(cat_key_level[row][col]);

    for (uint16_t run = cat_key_offsets[index]; run < cat_key_offsets[index + 1]; run++) {
        const uint8_t x      = cat_key_runs[run * 3];
        const uint8_t y      = cat_key_runs[run * 3 + 1];
        const uint8_t length = cat_key_runs[run * 3 + 2];
        for (uint8_t i = 0; i < length; i++) {
            if (cat_pixel_visible(x, y + i)) {
                draw_fill(x, y + i, x, y + i, color);
            }
        }
    }
}

/* Paw patches redraw the keys beneath them unlit, so lit keys go back on after a paw moves */
static void cat_keys_redraw_lit(void) {
    for (uint8_t row = 0; row < KEY_ROWS; row++) {
        for (uint8_t col = 0; col < KEY_COLS; col++) {
            if (cat_key_level[row][col]) {
                cat_key_draw(row, col);
            }
        }
    }
}

/* A press lights a key fully. On release its level stays up and the fade takes it down */
static void cat_keys_task(void) {
    for (uint8_t row = 0; row < KEY_ROWS; row++) {
        const uint16_t pressed = keys_state[row] & ~cat_keys_drawn[row];
        cat_keys_drawn[row]    = keys_state[row];
        if (!pressed) {
            continue;
        }
        for (uint8_t col = 0; col < KEY_COLS; col++) {
            if (pressed & (1u << col)) {
                cat_key_level[row][col] = CAT_KEY_LEVEL_MAX;
                cat_key_draw(row, col);
            }
        }
    }

    if (board_millis() - cat_fade_timer < CAT_KEY_FADE_STEP_MS) {
        return;
    }
    cat_fade_timer = board_millis();

    const uint8_t step = CAT_KEY_LEVEL_MAX * CAT_KEY_FADE_STEP_MS / CAT_KEY_FADE_MS;
    for (uint8_t row = 0; row < KEY_ROWS; row++) {
        for (uint8_t col = 0; col < KEY_COLS; col++) {
            uint8_t *level = &cat_key_level[row][col];
            if (*level && !(keys_state[row] & (1u << col))) {
                *level = *level > step ? *level - step : 0;
                cat_key_draw(row, col);
            }
        }
    }
}

static void paw_draw(uint8_t paw, bool down) {
    if (paw == PAW_LEFT) {
        draw_image_mapped(BONGO_LEFT_X, BONGO_LEFT_Y, down ? &bongo_left_down : &bongo_left_up, accent_map);
    } else {
        draw_image_mapped(BONGO_RIGHT_X, BONGO_RIGHT_Y, down ? &bongo_right_down : &bongo_right_up, accent_map);
    }
}

/* The number sits in the bottom left corner, with its label above it under the keyboard */
static uint8_t wpm_digits_top(void) {
    return LCD_HEIGHT - WPM_MARGIN - font_digits.height;
}

static void draw_number(uint8_t x, uint8_t y, uint16_t value) {
    char text[8];

    append_number(text, value > 999 ? 999 : value);
    draw_fill(x, y, x + draw_text_width(&font_digits, "000") - 1, y + font_digits.height - 1, COLOR_BLACK);
    draw_text(x, y, &font_digits, text, COLOR_WHITE, COLOR_BLACK);
}

/* A new value waits out the interval, except the first draw after entering the mode */
static bool wpm_due(void) {
    if (wpm_drawn == wpm || (wpm_drawn >= 0 && board_millis() - wpm_timer < WPM_UPDATE_MS)) {
        return false;
    }
    wpm_drawn = wpm;
    wpm_timer = board_millis();
    return true;
}

static void bongo_enter(void) {
    draw_image(0, 0, &bongo_base);
    paw_draw(PAW_LEFT, false);
    paw_draw(PAW_RIGHT, false);
    draw_text(WPM_MARGIN, wpm_digits_top() - WPM_LABEL_GAP - font_small.height, &font_small, "WPM", COLOR_WHITE, COLOR_BLACK);
    paw_drawn  = PAW_NONE;
    paw_target = PAW_NONE;
    wpm_drawn  = -1;
    bongo_time_drawn = -1;
    blink_closed = false;
    blink_step = 0;
    blink_timer = board_millis();
    for (uint8_t row = 0; row < KEY_ROWS; row++) {
        cat_keys_drawn[row] = 0;
        for (uint8_t col = 0; col < KEY_COLS; col++) {
            cat_key_level[row][col] = 0;
        }
    }
    intro_taps = BONGO_INTRO_TAPS;
    intro_timer = board_millis();
}

static bool any_key_held(void) {
    for (uint8_t row = 0; row < KEY_ROWS; row++) {
        if (keys_state[row]) {
            return true;
        }
    }
    return false;
}

/*
 * The board faces the cat, so the left of the real keyboard is on the right of the
 * screen. PAW_LEFT is the paw on that side, and each key gets the paw nearest to it.
 */
static uint8_t key_paw(uint8_t row, uint8_t col) {
    const key_rect_t key = key_rect(row, col, iso_layout);
    return key.x + key.width / 2 < KEY_CENTER_UNITS ? PAW_LEFT : PAW_RIGHT;
}

static void eye_draw_closed(uint8_t x, uint8_t y, const image_t *open) {
    draw_fill(x, y, x + open->width - 1, y + open->height - 1, COLOR_BLACK);
    draw_fill(x, y + BONGO_EYE_LID_OFFSET, x + open->width - 1, y + BONGO_EYE_LID_OFFSET + BONGO_EYE_LID_HEIGHT - 1, COLOR_WHITE);
}

static void eyes_draw(bool closed) {
    if (closed) {
        eye_draw_closed(BONGO_EYE_RIGHT_X, BONGO_EYE_RIGHT_Y, &bongo_eye_right);
        eye_draw_closed(BONGO_EYE_LEFT_X, BONGO_EYE_LEFT_Y, &bongo_eye_left);
    } else {
        draw_image(BONGO_EYE_RIGHT_X, BONGO_EYE_RIGHT_Y, &bongo_eye_right);
        draw_image(BONGO_EYE_LEFT_X, BONGO_EYE_LEFT_Y, &bongo_eye_left);
    }
    blink_closed = closed;
    blink_timer  = board_millis();
}

/* Blinks only while the cat is idle, at uneven gaps. The short gap makes a double blink */
static void bongo_blink_task(void) {
    static const uint16_t gaps_ms[] = {3500, 5000, 180, 4200};

    if (intro_taps || paw_target != PAW_NONE || paw_drawn != PAW_NONE || any_key_held()) {
        if (blink_closed) {
            eyes_draw(false);
        }
        blink_timer = board_millis();
        return;
    }
    if (blink_closed) {
        if (board_millis() - blink_timer >= BONGO_BLINK_CLOSED_MS) {
            eyes_draw(false);
            blink_step = (blink_step + 1) % (sizeof(gaps_ms) / sizeof(gaps_ms[0]));
        }
    } else if (board_millis() - blink_timer >= gaps_ms[blink_step]) {
        eyes_draw(true);
    }
}

static void bongo_task(void) {
    /* A few taps on entering the mode, even if the key that switched modes is still held */
    if (intro_taps && board_millis() - intro_timer >= BONGO_INTRO_STEP_MS) {
        intro_timer = board_millis();
        intro_taps--;
        ui_on_tap();
    }

    /* Holding backspace spams its paw. Otherwise the paw stays down while a key is held, and a quick tap still shows for BONGO_TAP_MS */
    if (!intro_taps && (keys_state[BONGO_SPAM_ROW] & (1u << BONGO_SPAM_COL))) {
        if (board_millis() - paw_timer >= BONGO_SPAM_MS) {
            paw_target = paw_target == PAW_NONE ? key_paw(BONGO_SPAM_ROW, BONGO_SPAM_COL) : PAW_NONE;
            paw_timer  = board_millis();
        }
    } else if (paw_target != PAW_NONE && (intro_taps || !any_key_held()) && board_millis() - paw_timer >= BONGO_TAP_MS) {
        paw_target = PAW_NONE;
    }

    if (paw_drawn != paw_target) {
        if (paw_drawn != PAW_NONE) {
            paw_draw(paw_drawn, false);
        }
        if (paw_target != PAW_NONE) {
            paw_draw(paw_target, true);
        }
        paw_drawn = paw_target;
        cat_keys_redraw_lit();
    }

    cat_keys_task();
    bongo_blink_task();

    if (wpm_due()) {
        draw_number(WPM_MARGIN, wpm_digits_top(), wpm);
    }
}

/* Graph mode: current and peak WPM above a scrolling history, newest on the right */

static void graph_draw(void) {
    uint16_t top_value = GRAPH_MIN_SCALE;
    for (uint8_t i = 0; i < GRAPH_WIDTH; i++) {
        if (history[i] > top_value) {
            top_value = history[i];
        }
    }
    const uint16_t scale = (top_value + GRAPH_GRID_WPM - 1) / GRAPH_GRID_WPM * GRAPH_GRID_WPM;

    /* Row 0 is the bottom. A row is a grid line where it crosses a multiple of the grid step */
    uint8_t grid[GRAPH_HEIGHT];
    grid[0] = 0;
    for (uint8_t row = 1; row < GRAPH_HEIGHT; row++) {
        grid[row] = (uint32_t)row * scale / GRAPH_HEIGHT / GRAPH_GRID_WPM != (uint32_t)(row - 1) * scale / GRAPH_HEIGHT / GRAPH_GRID_WPM;
    }

    lcd_stream_begin(GRAPH_LEFT, GRAPH_TOP, GRAPH_RIGHT, GRAPH_BOTTOM);
    for (uint8_t age = 0; age < GRAPH_WIDTH; age++) {
        const uint16_t value  = history[(history_head + GRAPH_WIDTH - age) % GRAPH_WIDTH];
        uint8_t        height = (uint8_t)((uint32_t)value * GRAPH_HEIGHT / scale);
        /* Zero and unfilled history still draw a line along the bottom */
        if (!height) {
            height = 1;
        }
        for (int16_t row = GRAPH_HEIGHT - 1; row >= 0; row--) {
            uint16_t color;
            if (row + 1 == height) {
                color = COLOR_WHITE;
            } else if (row < height) {
                color = accent_colors[accent];
            } else {
                color = grid[row] ? COLOR_GRID : COLOR_BLACK;
            }
            lcd_stream_pixel(color);
        }
    }
    lcd_stream_end();
}

static void graph_peak_render(void) {
    char text[12];

    append_number(append_text(text, "PEAK "), peak > 999 ? 999 : peak);
    draw_fill(GRAPH_LEFT + draw_text_width(&font_digits, "000"), GRAPH_PEAK_Y, GRAPH_RIGHT, GRAPH_PEAK_Y + font_small.height - 1, COLOR_BLACK);
    draw_right_aligned(GRAPH_PEAK_Y, text);
}

static void graph_enter(void) {
    draw_right_aligned(GRAPH_HEADER_Y, "WPM");
    wpm_drawn  = -1;
    peak_drawn = -1;
    graph_draw();
}

static void graph_task(void) {
    if (wpm_due()) {
        draw_number(GRAPH_LEFT, GRAPH_HEADER_Y, wpm);
    }
    if (peak_drawn != peak) {
        peak_drawn = peak;
        graph_peak_render();
    }
}

/* Runs in every mode, so the history is there when the graph is opened */
static void history_task(void) {
    if (board_millis() - history_timer < GRAPH_SAMPLE_MS) {
        return;
    }
    history_timer         = board_millis();
    history_head          = (history_head + 1) % GRAPH_WIDTH;
    history[history_head] = wpm;
    if (wpm > peak) {
        peak = wpm;
    }
    if (mode == MODE_GRAPH && !waiting_for_mode && !menu_active && !media_upload_active()) {
        graph_draw();
    }
}

static uint16_t display_temperature(uint8_t celsius, uint8_t units_bit) {
    return units & units_bit ? (uint16_t)(celsius * 9 / 5 + 32) : celsius;
}

/* System mode: CPU and GPU temperatures with a bar each, and the download rate */

static bool system_fresh(void) {
    return system_received && board_millis() - system_timer < SYSTEM_STALE_MS;
}

static int32_t system_value(uint8_t row) {
    if (!system_fresh()) {
        return -1;
    }
    return row == 0 ? system_cpu : row == 1 ? system_gpu : system_download;
}

static void system_draw_temperature(uint8_t top, int32_t value) {
    const uint32_t width = SYSTEM_BAR_RIGHT - SYSTEM_BAR_LEFT + 1;
    uint32_t       fill  = (uint32_t)value * width / SYSTEM_BAR_FULL;
    if (fill > width) {
        fill = width;
    }

    draw_number(SYSTEM_MARGIN, top + SYSTEM_VALUE_Y, display_temperature((uint8_t)value, UNITS_SYSTEM_F));
    draw_outline(SYSTEM_BAR_LEFT, top + SYSTEM_BAR_Y, SYSTEM_BAR_RIGHT, top + SYSTEM_BAR_Y + SYSTEM_BAR_HEIGHT - 1, COLOR_WHITE);
    if (fill) {
        draw_fill(SYSTEM_BAR_LEFT, top + SYSTEM_BAR_Y, SYSTEM_BAR_LEFT + fill - 1, top + SYSTEM_BAR_Y + SYSTEM_BAR_HEIGHT - 1, accent_colors[accent]);
    }
}

/* Sent in hundredths. Shown with one decimal, and the digit font has no dot of its own */
static void system_draw_download(uint8_t top, int32_t value) {
    char          whole[8];
    char          tenths[2] = {(char)('0' + value % 100 / 10), 0};
    const uint8_t y         = top + SYSTEM_VALUE_Y;
    uint8_t       x         = SYSTEM_MARGIN;

    append_number(whole, (uint16_t)(value / 100));
    draw_text(x, y, &font_digits, whole, COLOR_WHITE, COLOR_BLACK);
    x += draw_text_width(&font_digits, whole) + SYSTEM_DOT_GAP;
    draw_fill(x, y + font_digits.height - SYSTEM_DOT_SIZE, x + SYSTEM_DOT_SIZE - 1, y + font_digits.height - 1, COLOR_WHITE);
    x += SYSTEM_DOT_SIZE + SYSTEM_DOT_GAP;
    draw_text(x, y, &font_digits, tenths, COLOR_WHITE, COLOR_BLACK);
}

static void system_render_row(uint8_t row, int32_t value) {
    static const char *const labels[SYSTEM_ROWS] = {"CPU", "GPU", "DOWN"};
    const uint8_t            top                 = SYSTEM_TOP(row);

    draw_fill(0, top, LAST_X, SYSTEM_TOP(row + 1) - 1, COLOR_BLACK);
    draw_text(SYSTEM_MARGIN, top + SYSTEM_LABEL_Y, &font_small, labels[row], COLOR_WHITE, COLOR_BLACK);

    if (value < 0) {
        draw_text(SYSTEM_MARGIN, top + SYSTEM_VALUE_Y, &font_small, "NO DATA", COLOR_WHITE, COLOR_BLACK);
    } else if (row == SYSTEM_ROWS - 1) {
        draw_right_aligned(top + SYSTEM_LABEL_Y, "MB/s");
        system_draw_download(top, value);
    } else {
        draw_right_aligned(top + SYSTEM_LABEL_Y, units & UNITS_SYSTEM_F ? "F" : "C");
        system_draw_temperature(top, value);
    }
}

static void system_enter(void) {
    for (uint8_t row = 0; row < SYSTEM_ROWS; row++) {
        system_drawn[row] = -2;
    }
}

static void system_task(void) {
    for (uint8_t row = 0; row < SYSTEM_ROWS; row++) {
        int32_t value = system_value(row);
        if (system_drawn[row] != value) {
            system_drawn[row] = value;
            system_render_row(row, value);
        }
    }
}

/* Weather mode: the icon numbers are the ones zoom-sync sends, the images are stock art */

static const uint32_t weather_icons[] = {
    0x0850BC18, /* Day, clear: sun */
    0x085151CC, /* Day, partly cloudy: sun and cloud */
    0x0855D3D0, /* Day, partly rainy: sun, cloud and rain */
    0x0850C768, /* Night, partly cloudy: moon and cloud */
    0x0855DF20, /* Night, clear: moon */
    0x0855CA08, /* Cloudy: cloud */
    0x0854E48C, /* Rainy: cloud and rain */
    0x08506AF0, /* Snowfall: snowflake */
    0x08515AF0, /* Thunderstorm: cloud and lightning */
};

static void weather_render(void) {
    char text[20];

    clear_screen();
    if (!weather_received) {
        centered_small_text((LCD_HEIGHT - font_small.height) / 2, "NO DATA");
        return;
    }

    if (weather_icon < sizeof(weather_icons) / sizeof(weather_icons[0])) {
        uint8_t width, height;
        if (extflash_image_size(weather_icons[weather_icon], &width, &height)) {
            extflash_draw_image(weather_icons[weather_icon], (LCD_WIDTH - width * 2) / 2, WEATHER_ICON_AREA_TOP + (WEATHER_ICON_AREA_HEIGHT - height * 2) / 2, 2);
        }
    }

    const char   *unit = units & UNITS_WEATHER_F ? "F" : "C";
    const uint8_t unit_width = draw_text_width(&font_small, unit);

    append_number(text, display_temperature(weather_current, UNITS_WEATHER_F));
    const uint8_t width  = draw_text_width(&font_digits, text);
    const uint8_t x      = (LCD_WIDTH - width - WEATHER_DEGREE_GAP - WEATHER_DEGREE_SIZE - unit_width) / 2;
    const uint8_t degree = x + width + WEATHER_DEGREE_GAP;
    draw_text(x, WEATHER_TEMP_Y, &font_digits, text, COLOR_WHITE, COLOR_BLACK);
    draw_outline(degree, WEATHER_TEMP_Y, degree + WEATHER_DEGREE_SIZE - 1, WEATHER_TEMP_Y + WEATHER_DEGREE_SIZE - 1, COLOR_WHITE);
    draw_text(degree + WEATHER_DEGREE_SIZE, WEATHER_TEMP_Y + font_digits.height - font_small.height, &font_small, unit, COLOR_WHITE, COLOR_BLACK);

    append_number(append_text(append_number(append_text(text, "L "), display_temperature(weather_low, UNITS_WEATHER_F)), "   H "), display_temperature(weather_high, UNITS_WEATHER_F));
    centered_small_text(WEATHER_RANGE_Y, text);
}

static void weather_task(void) {
    if (weather_dirty) {
        weather_dirty = false;
        weather_render();
    }
}

/* Clock mode: four faces. Time comes from zoom-sync, and counts on between its updates */

static const int16_t clock_sine[60] = {
    0, 27, 53, 79, 104, 128, 150, 171, 190, 207, 222, 234, 243, 250, 255, 256, 255, 250, 243, 234, 222, 207, 190, 171, 150, 128, 104, 79, 53, 27, 0, -27, -53, -79, -104, -128, -150, -171, -190, -207, -222, -234, -243, -250, -255, -256, -255, -250, -243, -234, -222, -207, -190, -171, -150, -128, -104, -79, -53, -27
};

static uint32_t clock_seconds(void) {
    return clock_base_seconds + (board_millis() - clock_base_millis) / 1000;
}

static uint8_t clock_hour_12(uint32_t seconds) {
    const uint8_t hour = (uint8_t)(seconds / 3600 % 12);
    return hour ? hour : 12;
}

static const char *clock_weekday(void) {
    static const char *const names[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
    static const uint8_t     offsets[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    uint32_t                 year      = 2000u + clock_year;

    if (clock_month < 3) {
        year--;
    }
    return names[(year + year / 4 - year / 100 + year / 400 + offsets[clock_month - 1] + clock_day) % 7];
}

static void clock_date_text(char *text) {
    static const char *const months[] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};
    append_number(append_text(append_text(append_text(append_text(text, clock_weekday()), " "), months[clock_month - 1]), " "), clock_day);
}

static void draw_pixel(int16_t x, int16_t y, uint16_t color) {
    if (x >= 0 && y >= 0 && x < LCD_WIDTH && y < LCD_HEIGHT) {
        draw_fill((uint8_t)x, (uint8_t)y, (uint8_t)x, (uint8_t)y, color);
    }
}

static void draw_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) {
    const int16_t dx    = x1 > x0 ? x1 - x0 : x0 - x1;
    const int16_t dy    = y1 > y0 ? y0 - y1 : y1 - y0;
    const int16_t step_x = x0 < x1 ? 1 : -1;
    const int16_t step_y = y0 < y1 ? 1 : -1;
    int16_t       error = dx + dy;

    for (;;) {
        draw_pixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) {
            return;
        }
        const int16_t doubled = error * 2;
        if (doubled >= dy) {
            error += dy;
            x0 += step_x;
        }
        if (doubled <= dx) {
            error += dx;
            y0 += step_y;
        }
    }
}

static int16_t clock_x(uint8_t position, int16_t radius) {
    return CLOCK_CENTER + (int16_t)(clock_sine[position % 60] * radius / 256);
}

static int16_t clock_y(uint8_t position, int16_t radius) {
    return CLOCK_CENTER - (int16_t)(clock_sine[(position + 15) % 60] * radius / 256);
}

/* Width is 1 or 3, in pixels, drawn as parallel lines */
static void clock_draw_hand(uint8_t position, int16_t length, uint8_t width, uint16_t color) {
    const int16_t x = clock_x(position, length);
    const int16_t y = clock_y(position, length);

    draw_line(CLOCK_CENTER, CLOCK_CENTER, x, y, color);
    if (width > 1) {
        draw_line(CLOCK_CENTER + 1, CLOCK_CENTER, x + 1, y, color);
        draw_line(CLOCK_CENTER, CLOCK_CENTER + 1, x, y + 1, color);
    }
}

static void clock_draw_ticks(uint8_t step, int16_t inner, int16_t outer, uint16_t color) {
    for (uint8_t position = 0; position < 60; position += step) {
        draw_line(clock_x(position, inner), clock_y(position, inner), clock_x(position, outer), clock_y(position, outer), color);
    }
}

static uint8_t clock_hour_position(uint32_t seconds) {
    return (uint8_t)(seconds / 720 % 60);
}

/* Old hands are erased and all are redrawn, since a hand may have been crossing another */
static void clock_draw_analog(uint32_t seconds, bool detailed) {
    const uint8_t hour   = clock_hour_position(seconds);
    const uint8_t minute = (uint8_t)(seconds / 60 % 60);
    const uint8_t second = (uint8_t)(seconds % 60);

    if (clock_drawn_second != 0xFF) {
        if (detailed) {
            clock_draw_hand(clock_drawn_second, CLOCK_DETAILED_SECOND, 1, COLOR_BLACK);
        }
        clock_draw_hand(clock_drawn_minute, detailed ? CLOCK_DETAILED_MINUTE : CLOCK_SIMPLE_MINUTE, 3, COLOR_BLACK);
        clock_draw_hand(clock_drawn_hour, detailed ? CLOCK_DETAILED_HOUR : CLOCK_SIMPLE_HOUR, 3, COLOR_BLACK);
    }

    if (detailed) {
        char date[12];
        date[0] = 0;
        clock_date_text(date);
        centered_small_text(CLOCK_DATE_Y, date);
    }

    clock_draw_hand(hour, detailed ? CLOCK_DETAILED_HOUR : CLOCK_SIMPLE_HOUR, 3, COLOR_WHITE);
    clock_draw_hand(minute, detailed ? CLOCK_DETAILED_MINUTE : CLOCK_SIMPLE_MINUTE, 3, COLOR_WHITE);
    if (detailed) {
        clock_draw_hand(second, CLOCK_DETAILED_SECOND, 1, accent_colors[accent]);
    }
    draw_fill(CLOCK_CENTER - 2, CLOCK_CENTER - 2, CLOCK_CENTER + 2, CLOCK_CENTER + 2, COLOR_WHITE);

    clock_drawn_hour   = hour;
    clock_drawn_minute = minute;
    clock_drawn_second = second;
}

static void clock_draw_analog_dial(bool detailed) {
    if (!detailed) {
        clock_draw_ticks(5, CLOCK_SIMPLE_TICK, CLOCK_RADIUS, COLOR_WHITE);
        return;
    }

    clock_draw_ticks(1, CLOCK_RADIUS - 1, CLOCK_RADIUS, COLOR_GRID);
    clock_draw_ticks(5, CLOCK_RADIUS - 4, CLOCK_RADIUS, COLOR_WHITE);
    for (uint8_t number = 1; number <= 12; number++) {
        char          text[4];
        const uint8_t position = number * 5;
        append_number(text, number);
        draw_text((uint8_t)(clock_x(position, CLOCK_NUMERAL_RADIUS) - draw_text_width(&font_small, text) / 2), (uint8_t)(clock_y(position, CLOCK_NUMERAL_RADIUS) - font_small.height / 2), &font_small, text, COLOR_WHITE, COLOR_BLACK);
    }
}

static void clock_draw_digital(uint32_t seconds) {
    char text[16];

    /* The digits are fixed width, so only the seconds strip changes most of the time */
    const uint8_t minute = (uint8_t)(seconds / 60 % 60);
    if (minute != clock_drawn_minute) {
        const uint8_t hour   = clock_hour_12(seconds);
        const uint8_t digit  = draw_text_width(&font_digits, "0");
        const uint8_t left   = (LCD_WIDTH - digit * 4 - CLOCK_COLON_WIDTH + CLOCK_DIGIT_GAP) / 2;
        const uint8_t colon  = left + digit * 2 - CLOCK_DIGIT_GAP + (CLOCK_COLON_WIDTH + CLOCK_DIGIT_GAP - CLOCK_COLON_SIZE) / 2;

        draw_fill(0, CLOCK_DIGITAL_Y, LAST_X, CLOCK_DIGITAL_Y + font_digits.height - 1, COLOR_BLACK);
        append_two_digits(text, hour);
        draw_text(left, CLOCK_DIGITAL_Y, &font_digits, text, COLOR_WHITE, COLOR_BLACK);
        draw_fill(colon, CLOCK_DIGITAL_Y + 6, colon + CLOCK_COLON_SIZE - 1, CLOCK_DIGITAL_Y + 8, COLOR_WHITE);
        draw_fill(colon, CLOCK_DIGITAL_Y + 12, colon + CLOCK_COLON_SIZE - 1, CLOCK_DIGITAL_Y + 14, COLOR_WHITE);
        append_two_digits(text, minute);
        draw_text(left + digit * 2 + CLOCK_COLON_WIDTH, CLOCK_DIGITAL_Y, &font_digits, text, COLOR_WHITE, COLOR_BLACK);

        text[0] = 0;
        clock_date_text(text);
        draw_fill(0, CLOCK_DIGITAL_DATE_Y, LAST_X, CLOCK_DIGITAL_DATE_Y + font_small.height - 1, COLOR_BLACK);
        centered_small_text(CLOCK_DIGITAL_DATE_Y, text);
        clock_drawn_minute = minute;
    }

    /* A bar that fills over the minute, with the seconds below it */
    const uint8_t second = (uint8_t)(seconds % 60);
    const uint8_t fill   = (uint8_t)((uint32_t)(CLOCK_BAR_RIGHT - CLOCK_BAR_LEFT + 1) * (second + 1) / 60);

    draw_fill(CLOCK_BAR_LEFT, CLOCK_BAR_Y, CLOCK_BAR_RIGHT, CLOCK_BAR_Y + 2, COLOR_GRID);
    draw_fill(CLOCK_BAR_LEFT, CLOCK_BAR_Y, CLOCK_BAR_LEFT + fill - 1, CLOCK_BAR_Y + 2, accent_colors[accent]);
    append_two_digits(text, second);
    centered_small_text(CLOCK_BAR_Y + 6, text);
}

/* Each decimal digit is a column of bits, in pairs for hours, minutes and seconds */
static void clock_draw_binary(uint32_t seconds) {
    static const uint8_t max_bits[6] = {2, 4, 3, 4, 3, 4};
    const uint8_t        hour        = clock_hour_12(seconds);
    const uint8_t        minute      = (uint8_t)(seconds / 60 % 60);
    const uint8_t        second      = (uint8_t)(seconds % 60);
    const uint8_t        digits[6]   = {hour / 10, hour % 10, minute / 10, minute % 10, second / 10, second % 10};

    for (uint8_t column = 0; column < 6; column++) {
        if (digits[column] == clock_drawn_digits[column]) {
            continue;
        }
        clock_drawn_digits[column] = digits[column];

        const uint8_t pair = column / 2;
        const uint8_t x    = CLOCK_BINARY_LEFT + column * (CLOCK_CELL + CLOCK_CELL_GAP) + pair * CLOCK_PAIR_GAP;
        for (uint8_t bit = 0; bit < max_bits[column]; bit++) {
            const uint8_t y = CLOCK_BINARY_TOP + (3 - bit) * (CLOCK_CELL + CLOCK_CELL_GAP);
            if (digits[column] & (1u << bit)) {
                draw_fill(x, y, x + CLOCK_CELL - 1, y + CLOCK_CELL - 1, accent_colors[accent]);
            } else {
                draw_fill(x, y, x + CLOCK_CELL - 1, y + CLOCK_CELL - 1, COLOR_BLACK);
                draw_outline(x, y, x + CLOCK_CELL - 1, y + CLOCK_CELL - 1, COLOR_GRID);
            }
        }

        char text[2] = {(char)('0' + digits[column]), 0};
        draw_fill(x, CLOCK_BINARY_LABEL_Y, x + CLOCK_CELL - 1, CLOCK_BINARY_LABEL_Y + font_small.height - 1, COLOR_BLACK);
        draw_text(x + (CLOCK_CELL - draw_text_width(&font_small, text)) / 2, CLOCK_BINARY_LABEL_Y, &font_small, text, COLOR_WHITE, COLOR_BLACK);
    }
}

static void clock_enter(void) {
    clock_drawn_seconds = 0xFFFFFFFFu;
    clock_drawn_hour    = 0xFF;
    clock_drawn_minute  = 0xFF;
    clock_drawn_second  = 0xFF;
    clock_dial_drawn    = false;
    clock_empty_drawn   = false;
    for (uint8_t column = 0; column < 6; column++) {
        clock_drawn_digits[column] = 0xFF;
    }
}

static void clock_task(void) {
    if (!clock_valid) {
        if (!clock_empty_drawn) {
            clock_empty_drawn = true;
            centered_small_text((LCD_HEIGHT - font_small.height) / 2, "NO TIME");
        }
        return;
    }

    const uint32_t seconds = clock_seconds();
    if (seconds == clock_drawn_seconds) {
        return;
    }
    clock_drawn_seconds = seconds;

    if (!clock_dial_drawn) {
        clock_dial_drawn = true;
        clear_screen();
        if (clock_face == CLOCK_FACE_SIMPLE || clock_face == CLOCK_FACE_DETAILED) {
            clock_draw_analog_dial(clock_face == CLOCK_FACE_DETAILED);
        }
    }

    switch (clock_face) {
        case CLOCK_FACE_SIMPLE:
            if (seconds / 60 % 60 != clock_drawn_minute) {
                clock_draw_analog(seconds, false);
            }
            break;
        case CLOCK_FACE_DETAILED:
            clock_draw_analog(seconds, true);
            break;
        case CLOCK_FACE_DIGITAL:
            clock_draw_digital(seconds);
            break;
        default:
            clock_draw_binary(seconds);
            break;
    }
}

static uint8_t pill_width(void) {
    return (uint8_t)(PILL_PADDING * 2 + pill_count * PILL_DOT_SIZE + (pill_count - 1) * PILL_DOT_GAP);
}

static void pill_draw(void) {
    const uint8_t width  = pill_width();
    const uint8_t left   = (LCD_WIDTH - width) / 2;
    const uint8_t right  = left + width - 1;
    const uint8_t bottom = PILL_Y + PILL_HEIGHT - 1;

    /* An edge of black keeps it apart from whatever is behind it */
    draw_fill(left - 1, PILL_Y - 1, right + 1, bottom + 1, COLOR_BLACK);
    draw_fill(left + 2, PILL_Y, right - 2, bottom, COLOR_GRID);
    draw_fill(left, PILL_Y + 2, left + 1, bottom - 2, COLOR_GRID);
    draw_fill(right - 1, PILL_Y + 2, right, bottom - 2, COLOR_GRID);
    draw_fill(left + 1, PILL_Y + 1, left + 1, bottom - 1, COLOR_GRID);
    draw_fill(right - 1, PILL_Y + 1, right - 1, bottom - 1, COLOR_GRID);

    for (uint8_t dot = 0; dot < pill_count; dot++) {
        const uint8_t  x     = (uint8_t)(left + PILL_PADDING + dot * (PILL_DOT_SIZE + PILL_DOT_GAP));
        const uint8_t  y     = PILL_Y + (PILL_HEIGHT - PILL_DOT_SIZE) / 2;
        const uint16_t color = dot == pill_selected ? accent_colors[accent] : COLOR_DIM;
        draw_fill(x + 1, y, x + PILL_DOT_SIZE - 2, y + PILL_DOT_SIZE - 1, color);
        draw_fill(x, y + 1, x + PILL_DOT_SIZE - 1, y + PILL_DOT_SIZE - 2, color);
    }
}

static void pill_show(uint8_t count, uint8_t selected) {
    pill_count    = count;
    pill_selected = selected;
    pill_timer    = board_millis();
    pill_active   = true;
    pill_draw();
}

/* The page comes back in place, with no clear, so nothing around the pill flickers */
static void pill_hide(void) {
    const uint8_t width = pill_width();
    const uint8_t left  = (LCD_WIDTH - width) / 2;

    pill_active = false;
    if (mode == MODE_MEDIA) {
        media_page_redraw();
        return;
    }
    draw_fill(left - 1, PILL_Y - 1, left + width, PILL_Y + PILL_HEIGHT, COLOR_BLACK);
    if (mode == MODE_CLOCK && (clock_face == CLOCK_FACE_SIMPLE || clock_face == CLOCK_FACE_DETAILED)) {
        clock_draw_analog_dial(clock_face == CLOCK_FACE_DETAILED);
        clock_drawn_seconds = 0xFFFFFFFFu;
        clock_drawn_minute  = 0xFF;
    }
}

static void pill_task(bool page_drew) {
    if (!pill_active) {
        return;
    }
    if (board_millis() - pill_timer >= PILL_SHOW_MS) {
        pill_hide();
    } else if (page_drew) {
        pill_draw();
    }
}

static void enter_mode(void) {
    pill_active = false;
    clear_screen();
    switch (mode) {
        case MODE_BONGO:
            bongo_enter();
            break;
        case MODE_STATUS:
            status_enter();
            break;
        case MODE_GRAPH:
            graph_enter();
            break;
        case MODE_SYSTEM:
            system_enter();
            break;
        case MODE_WEATHER:
            weather_dirty = true;
            break;
        case MODE_RGB:
            rgb_dirty = true;
            break;
        case MODE_CLOCK:
            clock_enter();
            break;
        case MODE_MEDIA:
            media_page_enter(media_show_gif);
            break;
        default:
            break;
    }
}

/* The time sits in the bottom right corner, level with the bottom of the WPM number */
static void bongo_time_task(void) {
    if (!clock_valid) {
        return;
    }
    const uint32_t seconds = clock_seconds();
    if (bongo_time_drawn == (int32_t)(seconds / 60)) {
        return;
    }
    bongo_time_drawn = (int32_t)(seconds / 60);

    char text[8];
    text[0] = 0;
    append_two_digits(append_text(append_two_digits(text, clock_hour_12(seconds)), ":"), seconds / 60 % 60);
    draw_fill(BONGO_TIME_LEFT, BONGO_TIME_Y, LAST_X, BONGO_TIME_Y + font_small.height - 1, COLOR_BLACK);
    draw_text(BONGO_TIME_RIGHT - draw_text_width(&font_small, text), BONGO_TIME_Y, &font_small, text, COLOR_WHITE, COLOR_BLACK);
}

static void menu_draw_row(uint8_t row, bool selected) {
    static const char *const names[MODE_COUNT] = {
        [MODE_CLOCK] = "CLOCK", [MODE_BONGO] = "BONGO CAT", [MODE_GRAPH] = "WPM GRAPH", [MODE_SYSTEM] = "SYSTEM STATS",
        [MODE_WEATHER] = "WEATHER", [MODE_RGB] = "RGB STATUS", [MODE_MEDIA] = "IMAGE / GIF", [MODE_STATUS] = "KEYBOARD INFO",
    };
    const uint8_t  top    = MENU_TOP(row);
    const uint8_t  bottom = MENU_TOP(row + 1) - 1;
    const uint8_t  y      = top + (bottom - top + 1 - font_small.height) / 2;
    const uint8_t  right  = LAST_X - MENU_BAR_MARGIN;
    const uint16_t bg     = selected ? accent_colors[accent] : COLOR_BLACK;
    const uint16_t fg     = selected ? COLOR_BLACK : COLOR_WHITE;

    draw_fill(0, top, LAST_X, bottom, COLOR_BLACK);
    if (selected) {
        draw_fill(MENU_BAR_MARGIN, top, right, bottom - 1, bg);
        draw_fill(MENU_BAR_MARGIN, top, MENU_BAR_MARGIN, top, COLOR_BLACK);
        draw_fill(right, top, right, top, COLOR_BLACK);
        draw_fill(MENU_BAR_MARGIN, bottom - 1, MENU_BAR_MARGIN, bottom - 1, COLOR_BLACK);
        draw_fill(right, bottom - 1, right, bottom - 1, COLOR_BLACK);
        draw_text(MENU_MARKER_X, y, &font_small, ">", fg, bg);
    }
    draw_text(MENU_TEXT_X, y, &font_small, names[row], fg, bg);
}

/* Further presses while it is open move the selection and restart the timer */
static void menu_show(void) {
    if (menu_active) {
        menu_draw_row(menu_drawn, false);
    } else {
        pill_active = false;
        for (uint8_t row = 0; row < MODE_COUNT; row++) {
            menu_draw_row(row, false);
        }
        menu_active = true;
    }
    menu_draw_row(mode, true);
    menu_drawn = mode;
    menu_timer = board_millis();
}

/* The screen stays black until the keyboard says which mode to show, so no other mode flashes up */
void ui_init(void) {
    start_time = board_millis();
}

/* In reactive mode the GIF only plays while keys are held or were just pressed */
static bool media_playing(void) {
    return !media_reactive || any_key_held() || board_millis() - media_key_timer < MEDIA_REACTIVE_MS;
}

void ui_task(void) {
    media_task();

    if (!powered) {
        return;
    }

    history_task();

    if (media_upload_active()) {
        return;
    }

    if (waiting_for_mode) {
        if (board_millis() - start_time < FIRST_MODE_WAIT_MS) {
            return;
        }
        waiting_for_mode = false;
        enter_mode();
    }

    if (menu_active) {
        if (board_millis() - menu_timer >= MENU_SHOW_MS) {
            menu_active = false;
            enter_mode();
        }
        return;
    }

    switch (mode) {
        case MODE_BONGO:
            bongo_task();
            bongo_time_task();
            break;
        case MODE_STATUS:
            status_task();
            break;
        case MODE_GRAPH:
            graph_task();
            break;
        case MODE_SYSTEM:
            system_task();
            break;
        case MODE_WEATHER:
            weather_task();
            break;
        case MODE_RGB:
            rgb_task();
            break;
        case MODE_CLOCK: {
            const uint32_t drawn = clock_drawn_seconds;
            clock_task();
            pill_task(clock_drawn_seconds != drawn);
            break;
        }
        case MODE_MEDIA:
            media_page_task(media_playing());
            pill_task(media_page_take_drawn());
            break;
        default:
            break;
    }
}

void ui_on_tap(void) {
    paw_target = paw_next;
    paw_next   = paw_next == PAW_LEFT ? PAW_RIGHT : PAW_LEFT;
    paw_timer  = board_millis();
}

void ui_on_key_tap(uint8_t row, uint8_t col) {
    media_key_timer = board_millis();
    status_key_count++;
    if (row >= KEY_ROWS || col >= KEY_COLS || !key_rect(row, col, iso_layout).width) {
        ui_on_tap();
        return;
    }

    paw_target = key_paw(row, col);
    paw_timer  = board_millis();
}

void ui_on_wpm(uint16_t value) {
    wpm = value;
}

void ui_on_status(uint8_t device, uint8_t layer, bool caps, uint8_t battery) {
    status_device  = device;
    status_layer   = layer;
    status_caps    = caps;
    status_battery = battery;
}

void ui_on_status_extra(uint8_t flags) {
    status_flags          = flags;
    status_extra_received = true;
}

void ui_on_rgb(bool enabled, uint8_t effect, uint8_t hue, uint8_t saturation, uint8_t value, uint8_t speed) {
    if (rgb_received && enabled == rgb_enabled && effect == rgb_mode && hue == rgb_hue && saturation == rgb_saturation && value == rgb_value && speed == rgb_speed) {
        return;
    }
    rgb_received   = true;
    rgb_enabled    = enabled;
    rgb_mode       = effect;
    rgb_hue        = hue;
    rgb_saturation = saturation;
    rgb_value      = value;
    rgb_speed      = speed;
    rgb_dirty      = true;
}

void ui_on_rgb_name(const char *name) {
    for (uint8_t i = 0; i < RGB_NAME_SIZE; i++) {
        if (rgb_name[i] != name[i]) {
            rgb_dirty = true;
        }
        rgb_name[i] = name[i];
    }
}

void ui_on_versions(const char *qmk, uint16_t via) {
    for (uint8_t i = 0; i < STATUS_QMK_VERSION_SIZE - 1; i++) {
        status_qmk_version[i] = qmk[i];
    }
    status_via_version  = via;
    status_qmk_received = true;
}

void ui_on_build(const char *build) {
    for (uint8_t i = 0; i < STATUS_BUILD_FIELD_SIZE; i++) {
        status_build_date[i] = build[i];
        status_build_hash[i] = build[STATUS_BUILD_FIELD_SIZE + i];
    }
    status_build_received = true;
}

void ui_on_keys(uint8_t row, uint16_t state) {
    if (row < KEY_ROWS) {
        keys_state[row] = state;
    }
}

/* Holds the screen for the length of an upload, which also keeps flash reads away from it */
static void upload_draw_bar(void) {
    const uint8_t width = (uint8_t)((uint32_t)(UPLOAD_BAR_RIGHT - UPLOAD_BAR_LEFT - 1) * upload_percent / 100);

    if (width) {
        draw_fill(UPLOAD_BAR_LEFT + 1, UPLOAD_BAR_Y + 1, UPLOAD_BAR_LEFT + width, UPLOAD_BAR_Y + UPLOAD_BAR_HEIGHT - 2, accent_colors[accent]);
    }
}

static void upload_draw(void) {
    clear_screen();
    centered_small_text(UPLOAD_TEXT_Y, upload_gif ? "UPLOADING GIF" : "UPLOADING IMAGE");
    draw_outline(UPLOAD_BAR_LEFT, UPLOAD_BAR_Y, UPLOAD_BAR_RIGHT, UPLOAD_BAR_Y + UPLOAD_BAR_HEIGHT - 1, COLOR_WHITE);
    upload_draw_bar();
}

void ui_upload_begin(bool gif) {
    upload_gif     = gif;
    upload_percent = 0;
    menu_active    = false;
    pill_active    = false;
    if (powered) {
        upload_draw();
    }
}

void ui_upload_progress(uint8_t percent) {
    if (percent != upload_percent) {
        upload_percent = percent;
        if (powered) {
            upload_draw_bar();
        }
    }
}

void ui_upload_end(void) {
    if (powered && !waiting_for_mode) {
        enter_mode();
    }
}

void ui_media_changed(void) {
    if (powered && mode == MODE_MEDIA && !waiting_for_mode && !menu_active && !media_upload_active()) {
        enter_mode();
    }
}

void ui_on_media(uint8_t setting) {
    const bool visible = powered && mode == MODE_MEDIA && !waiting_for_mode && !menu_active && !media_upload_active();

    if (setting >= MEDIA_SETTING_COUNT || setting == media_setting) {
        return;
    }
    media_setting  = setting;
    media_reactive = setting == MEDIA_SETTING_REACTIVE;
    if ((setting != MEDIA_SETTING_IMAGE) != media_show_gif) {
        media_show_gif = setting != MEDIA_SETTING_IMAGE;
        if (visible) {
            media_page_enter(media_show_gif);
        }
    }
    if (visible) {
        pill_show(MEDIA_SETTING_COUNT, setting);
    }
}

void ui_on_mode(uint8_t value) {
    if (value >= MODE_COUNT || (value == mode && !waiting_for_mode)) {
        return;
    }
    const bool first_mode = waiting_for_mode;

    mode             = value;
    waiting_for_mode = false;
    if (!powered || media_upload_active()) {
        return;
    }
    if (first_mode) {
        enter_mode();
    } else {
        menu_show();
    }
}

void ui_on_power(bool on) {
    if (on == powered) {
        return;
    }
    powered     = on;
    menu_active = false;
    lcd_set_power(on);
    if (on && media_upload_active()) {
        upload_draw();
    } else if (on && !waiting_for_mode) {
        enter_mode();
    }
}

void ui_on_system(uint8_t cpu, uint8_t gpu, uint16_t download) {
    system_cpu      = cpu;
    system_gpu      = gpu;
    system_download = download;
    system_received = true;
    system_timer    = board_millis();
}

void ui_on_weather(uint8_t icon, uint8_t current, uint8_t low, uint8_t high) {
    if (weather_received && icon == weather_icon && current == weather_current && low == weather_low && high == weather_high) {
        return;
    }
    weather_icon     = icon;
    weather_current  = current;
    weather_low      = low;
    weather_high     = high;
    weather_received = true;
    weather_dirty    = true;
}

void ui_on_backlight(bool on) {
    lcd_set_backlight(on);
}

void ui_on_brightness(uint8_t percent) {
    lcd_set_brightness(percent);
}

bool ui_needs_speed(void) {
    return powered && mode == MODE_MEDIA && media_show_gif && !waiting_for_mode && !menu_active && media_playing();
}

bool ui_can_sleep(void) {
    return !powered && !media_upload_active();
}

void ui_on_accent(uint8_t value) {
    if (value >= ACCENT_COUNT || value == accent) {
        return;
    }
    accent = value;
    if (powered && mode == MODE_BONGO && !waiting_for_mode && !menu_active && !media_upload_active()) {
        paw_draw(PAW_LEFT, paw_drawn == PAW_LEFT);
        paw_draw(PAW_RIGHT, paw_drawn == PAW_RIGHT);
        cat_keys_redraw_lit();
    }
    if (powered && mode == MODE_STATUS && !waiting_for_mode && !menu_active && !media_upload_active()) {
        status_enter();
    }
    if (powered && mode == MODE_GRAPH && !waiting_for_mode && !menu_active && !media_upload_active()) {
        graph_draw();
    }
}

void ui_on_time(uint8_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second) {
    if (month < 1 || month > 12 || day < 1 || day > 31) {
        return;
    }
    clock_year         = year;
    clock_month        = month;
    clock_day          = day;
    clock_base_seconds = (uint32_t)hour * 3600 + (uint32_t)minute * 60 + second;
    clock_base_millis  = board_millis();
    clock_valid        = true;

    /* Replaces "NO TIME", and a correction can move any hand, so the page starts over */
    if (mode == MODE_CLOCK && powered && !waiting_for_mode && !menu_active && !media_upload_active()) {
        clock_enter();
    }
}

void ui_on_clock_face(uint8_t value) {
    if (value >= CLOCK_FACE_COUNT || value == clock_face) {
        return;
    }
    clock_face = value;
    if (mode == MODE_CLOCK && powered && !waiting_for_mode && !menu_active && !media_upload_active()) {
        clear_screen();
        clock_enter();
        pill_show(CLOCK_FACE_COUNT, value);
    }
}

void ui_on_units(uint8_t value) {
    if (value == units) {
        return;
    }
    units = value;
    system_enter();
    weather_dirty = true;
}

void ui_on_layout(bool iso) {
    iso_layout = iso;
}
