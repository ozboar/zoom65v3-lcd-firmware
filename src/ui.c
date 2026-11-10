#include <string.h>

#include "assets.h"
#include "board.h"
#include "draw.h"
#include "extflash.h"
#include "gif.h"
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

/*
 * Page menu: a drawer that slides in over the left of the page when switching pages. Pages
 * slide behind it as the selection moves, and it slides away once the selection settles
 */
#define MENU_SHOW_MS 800
#define MENU_BAR_MARGIN 3
#define MENU_WIDTH 59
#define MENU_STRIP_WIDTH (LCD_WIDTH - MENU_WIDTH)
#define MENU_SLIDE_MS 150
#define PAGE_SLIDE_MS 150

/* Changing the accent fades the page from the old color to the new one */
#define ACCENT_FADE_MS 250
#define FADE_LEVELS 32
#define EASE_ONE 1024u

/*
 * The drawer is frosted glass: the page under it shows through blurred and darkened, and through
 * the highlight tinted with the accent. Foreground pixels in the key color are see-through
 */
#define MENU_KEY_CLEAR 0x0001
#define MENU_MARKER_WIDTH 6
#define MENU_MARKER_HEIGHT 16
#define MENU_TINT_LEVEL 24
#define BLUR_WIDTH ((MENU_WIDTH + 1) / 2)
#define BLUR_HEIGHT (LCD_HEIGHT / 2)
#define BLUR_RADIUS 3
#define BLUR_PASSES 2
#define BLUR_MAX_SIDE (LCD_WIDTH / 2)

/*
 * A page shown briefly as a frosted popup over the current one, which holds still underneath. Its
 * black shows the current page blurred, and its grey and white blend over that like light
 */
#define PEEK_SHOW_MS 800
#define POPUP_FADE_MS 200
#define POPUP_BLUR_SIDE (LCD_WIDTH / 2)

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
#define STATUS_LABEL_GAP 2
#define STATUS_QMK_Y 92
#define STATUS_QMK_VERSION_SIZE 18
#define STATUS_BUILD_Y 106
#define STATUS_BUILD_FIELD_SIZE 10
#define STATUS_BATTERY_LOW 20
#define STATUS_CHARGE_STEPS 4
#define STATUS_CHARGE_STEP_MS 500
#define BATTERY_WARN_PERCENT 10
#define BATTERY_WARN_CLEAR_PERCENT 15
/* Pages redraw over the icon, so this is how long it can stay hidden */
#define BATTERY_WARN_REPAINT_MS 50
#define BATTERY_WARN_X 52
#define BATTERY_WARN_Y 1
#define BATTERY_WARN_WIDTH 13
#define BATTERY_WARN_HEIGHT 7
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
static uint8_t  menu_bar_from;
static bool               menu_right;
static const char *const *menu_names;
static uint8_t            menu_count;
static uint8_t            menu_target;
static uint8_t  menu_marker[MENU_MARKER_WIDTH * MENU_MARKER_HEIGHT * 2];
static uint32_t menu_timer;

typedef enum { MENU_OPENING, MENU_OPEN, MENU_CLOSING } menu_state_t;

static menu_state_t menu_state;
static uint8_t      menu_edge;
static uint8_t      menu_edge_from;
static uint32_t     menu_slide_timer;
static bool         page_slide_pending;
static bool         page_slide_active;
static bool         page_slide_down;
static uint8_t      page_slide_drawn;
static uint32_t     page_slide_timer;

/*
 * A screen drawn off screen for the slides. The menu uses it as two bands: the strip right of the
 * drawer, old and new, or a whole page split at the drawer's edge. A tab slide uses it whole for
 * the old view. The ROM bootloader's SRAM is free for this once running
 */
_Static_assert(2 * MENU_WIDTH == LCD_WIDTH, "the two bands make up one screen");
static uint8_t page_frame[LCD_WIDTH * LCD_HEIGHT * 2] __attribute__((section(".low_ram")));
#define page_band(index) (page_frame + (index) * MENU_WIDTH * LCD_HEIGHT * 2)
static uint8_t band_shown;

/* The drawer's foreground, and the blurred page under it for each band's page */
static uint8_t  menu_foreground[MENU_WIDTH * LCD_HEIGHT * 2] __attribute__((section(".media_ram")));
static uint16_t menu_blur[2][BLUR_WIDTH * BLUR_HEIGHT] __attribute__((section(".media_ram")));

/* The accent fade, between the old view in the page frame and the new one in the GIF canvas */
static bool     fade_active;
static uint8_t  fade_drawn;
static uint32_t fade_timer;
static uint8_t  fade_top;
static uint8_t  fade_bottom;

typedef enum { POPUP_FADING_IN, POPUP_SHOWN, POPUP_FADING_OUT } popup_state_t;

static bool          popup_active;
static bool          popup_dirty;
static popup_state_t popup_state;
static uint8_t       popup_mode;
static uint8_t       popup_level;
static uint8_t       popup_level_from;
static uint32_t      popup_timer;
static uint32_t      popup_fade_timer;
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

/* The reading is the battery's voltage, which charging pushes up, so on the cable only the charger's state shows */
static bool status_plugged(void) {
    return status_extra_received && (status_flags & STATUS_FLAG_CHARGING);
}

static bool status_full(void) {
    return status_plugged() && (status_flags & STATUS_FLAG_FULL);
}

/* While charging the battery fills up step by step, then starts over */
static uint8_t status_charge_step(void) {
    return (board_millis() / STATUS_CHARGE_STEP_MS) % (STATUS_CHARGE_STEPS + 1);
}

static void status_draw_header(void) {
    char          text[6];
    const uint8_t right  = STATUS_MARGIN + STATUS_ICON_WIDTH - 1;
    const uint8_t bottom = STATUS_HEADER_Y + STATUS_ICON_HEIGHT - 1;
    const uint8_t width  = STATUS_ICON_WIDTH - 4;
    uint32_t      percent;

    if (status_full()) {
        percent = 100;
        append_text(text, "FULL");
    } else if (status_plugged()) {
        percent = status_charge_step() * 100u / STATUS_CHARGE_STEPS;
        append_text(text, "CHG");
    } else {
        percent = status_battery > 100 ? 100 : status_battery;
        append_text(append_number(text, status_battery), "%");
    }

    const uint8_t fill = (uint8_t)((uint32_t)width * percent / 100);
    const bool    low  = !status_plugged() && percent < STATUS_BATTERY_LOW;

    /* A filled band, with the battery and text in black on it */
    draw_fill(0, 0, LAST_X, STATUS_DIVIDER_Y, accent_colors[accent]);
    draw_outline(STATUS_MARGIN, STATUS_HEADER_Y, right, bottom, COLOR_BLACK);
    draw_fill(right + 1, STATUS_HEADER_Y + 3, right + 2, bottom - 3, COLOR_BLACK);
    if (fill) {
        draw_fill(STATUS_MARGIN + 2, STATUS_HEADER_Y + 2, STATUS_MARGIN + 1 + fill, bottom - 2, low ? COLOR_LOW : COLOR_BLACK);
    }
    draw_text(right + 5, STATUS_HEADER_Y, &font_small, text, COLOR_BLACK, accent_colors[accent]);
}

/* Changes whenever the header looks different, including each step of the charging fill */
static int32_t status_header_state(void) {
    if (status_full()) {
        return 1 << 16;
    }
    if (status_plugged()) {
        return 2 << 16 | status_charge_step();
    }
    return status_battery;
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
    /* Right aligned, unless that would run into the label */
    const int16_t aligned = STATUS_LEFT_VALUE_EDGE - draw_text_width(&font_small, value);
    const int16_t minimum = STATUS_MARGIN + draw_text_width(&font_small, label) + STATUS_LABEL_GAP;
    draw_text(aligned < minimum ? minimum : aligned, y, &font_small, value, COLOR_WHITE, COLOR_BLACK);
}

static void status_draw_right(uint8_t y, const char *label, const char *value) {
    draw_fill(STATUS_UPTIME_X, y, LAST_X, y + font_small.height - 1, COLOR_BLACK);
    draw_text(STATUS_UPTIME_X, y, &font_small, label, COLOR_DIM, COLOR_BLACK);
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
        case STATUS_ROW_BUILD:
            status_draw_left(STATUS_BUILD_Y, "BUILD", status_build_hash);
            status_draw_right(STATUS_BUILD_Y, "LCD", LCD_VERSION);
            break;
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
    const int32_t header = status_header_state();
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

    /* Newest at the right. Zero and unfilled history still draw a line along the bottom */
    uint8_t heights[GRAPH_WIDTH];
    for (uint8_t age = 0; age < GRAPH_WIDTH; age++) {
        const uint16_t value = history[(history_head + GRAPH_WIDTH - age) % GRAPH_WIDTH];
        const uint8_t  bar   = (uint8_t)((uint32_t)value * GRAPH_HEIGHT / scale);
        heights[GRAPH_WIDTH - 1 - age] = bar ? bar : 1;
    }

    lcd_stream_begin(GRAPH_LEFT, GRAPH_TOP, GRAPH_RIGHT, GRAPH_BOTTOM);
    for (int16_t row = GRAPH_HEIGHT - 1; row >= 0; row--) {
        for (uint8_t col = 0; col < GRAPH_WIDTH; col++) {
            const uint8_t height = heights[col];
            uint16_t      color;
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
    if (mode == MODE_GRAPH && !waiting_for_mode && !menu_active && !popup_active && !media_upload_active()) {
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

static void enter_mode(void) {
    fade_active = false;
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

static const char *const page_names[MODE_COUNT] = {
    [MODE_CLOCK] = "CLOCK", [MODE_BONGO] = "CAT", [MODE_GRAPH] = "WPM", [MODE_SYSTEM] = "SYSTEM",
    [MODE_WEATHER] = "WEATHER", [MODE_RGB] = "RGB", [MODE_MEDIA] = "MEDIA", [MODE_STATUS] = "INFO",
};
static const char *const clock_face_names[CLOCK_FACE_COUNT] = {
    [CLOCK_FACE_DETAILED] = "DETAILED", [CLOCK_FACE_SIMPLE] = "SIMPLE", [CLOCK_FACE_DIGITAL] = "DIGITAL", [CLOCK_FACE_BINARY] = "BINARY",
};
static const char *const media_setting_names[MEDIA_SETTING_COUNT] = {"IMAGE", "GIF", "REACTIVE"};

/* Rows are as tall as the page menu's, with a shorter list in the middle of the drawer */
static uint8_t menu_row_top(uint8_t row) {
    const uint8_t top = (uint8_t)((LCD_HEIGHT - menu_count * LCD_HEIGHT / MODE_COUNT) / 2);
    return (uint8_t)(top + row * LCD_HEIGHT / MODE_COUNT);
}

/* The drawer's columns inside its divider, which faces the page */
static uint8_t menu_inner_left(void) {
    return menu_right ? 1 : 0;
}

/* How far the page behind moves: down the screen behind the page menu, across the strip behind the tabs */
static uint8_t page_slide_distance(void) {
    return menu_right ? MENU_STRIP_WIDTH : LCD_HEIGHT;
}

/* Drawn into the drawer's foreground in white on the see-through key. The highlight is added when drawing */
static void menu_render_foreground(void) {
    const uint8_t left = menu_inner_left();

    lcd_capture_begin(menu_foreground, 0, MENU_WIDTH);
    draw_fill(0, 0, MENU_WIDTH - 1, LAST_Y, MENU_KEY_CLEAR);
    for (uint8_t row = 0; row < menu_count; row++) {
        const uint8_t top    = menu_row_top(row);
        const uint8_t height = menu_row_top(row + 1) - top;
        draw_text(left + MENU_TEXT_X, top + (height - font_small.height) / 2, &font_small, menu_names[row], COLOR_WHITE, MENU_KEY_CLEAR);
    }
    draw_fill(menu_right ? 0 : MENU_WIDTH - 1, 0, menu_right ? 0 : MENU_WIDTH - 1, LAST_Y, COLOR_GRID);
    lcd_capture_end();
}

/* The highlight's marker, which moves with it */
static void menu_render_marker(void) {
    lcd_capture_begin(menu_marker, 0, MENU_MARKER_WIDTH);
    draw_fill(0, 0, MENU_MARKER_WIDTH - 1, font_small.height - 1, MENU_KEY_CLEAR);
    draw_text(0, 0, &font_small, ">", COLOR_WHITE, MENU_KEY_CLEAR);
    lcd_capture_end();
}

/* Eases out over duration, from 0 to distance */
static uint8_t ease_out(uint32_t elapsed, uint32_t duration, uint8_t distance) {
    if (elapsed >= duration) {
        return distance;
    }
    const uint32_t left = (duration - elapsed) * EASE_ONE / duration;
    const uint32_t cube = left * left / EASE_ONE * left / EASE_ONE;
    return (uint8_t)(distance - distance * cube / EASE_ONE);
}

static uint16_t pixel_at(const uint8_t *pixels, uint32_t index) {
    return (uint16_t)((pixels[index * 2] << 8) | pixels[index * 2 + 1]);
}

static uint16_t mix_color(uint16_t from, uint16_t to, uint8_t level) {
    const int32_t r = (int32_t)(from >> 11) + ((int32_t)(to >> 11) - (from >> 11)) * level / FADE_LEVELS;
    const int32_t g = (int32_t)((from >> 5) & 0x3F) + ((int32_t)((to >> 5) & 0x3F) - ((from >> 5) & 0x3F)) * level / FADE_LEVELS;
    const int32_t b = (int32_t)(from & 0x1F) + ((int32_t)(to & 0x1F) - (from & 0x1F)) * level / FADE_LEVELS;
    return (uint16_t)((r << 11) | (g << 5) | b);
}

/* One pass of a box blur along a line of count pixels, step apart */
static void blur_line(uint16_t *pixels, uint8_t count, uint8_t step) {
    uint16_t line[BLUR_MAX_SIDE];

    for (uint8_t i = 0; i < count; i++) {
        line[i] = pixels[i * step];
    }
    for (uint8_t i = 0; i < count; i++) {
        const uint8_t first = i > BLUR_RADIUS ? i - BLUR_RADIUS : 0;
        const uint8_t last  = i + BLUR_RADIUS < count ? i + BLUR_RADIUS : count - 1;
        uint32_t      r = 0, g = 0, b = 0;
        for (uint8_t j = first; j <= last; j++) {
            r += line[j] >> 11;
            g += (line[j] >> 5) & 0x3F;
            b += line[j] & 0x1F;
        }
        const uint8_t taps = last - first + 1;
        pixels[i * step]   = (uint16_t)((((r + taps / 2) / taps) << 11) | (((g + taps / 2) / taps) << 5) | ((b + taps / 2) / taps));
    }
}

/* A width by height image of big endian pixels, halved and blurred */
static void blur_image(const uint8_t *pixels, uint8_t width, uint8_t height, uint16_t *blur) {
    const uint8_t blur_width  = (width + 1) / 2;
    const uint8_t blur_height = height / 2;

    for (uint8_t by = 0; by < blur_height; by++) {
        for (uint8_t bx = 0; bx < blur_width; bx++) {
            uint32_t r = 0, g = 0, b = 0, taps = 0;
            for (uint8_t y = by * 2; y < by * 2 + 2 && y < height; y++) {
                for (uint8_t x = bx * 2; x < bx * 2 + 2 && x < width; x++) {
                    const uint16_t color = pixel_at(pixels, (uint32_t)y * width + x);
                    r += color >> 11;
                    g += (color >> 5) & 0x3F;
                    b += color & 0x1F;
                    taps++;
                }
            }
            blur[by * blur_width + bx] = (uint16_t)(((r / taps) << 11) | ((g / taps) << 5) | (b / taps));
        }
    }
    for (uint8_t pass = 0; pass < BLUR_PASSES; pass++) {
        for (uint8_t by = 0; by < blur_height; by++) {
            blur_line(blur + by * blur_width, blur_width, 1);
        }
        for (uint8_t bx = 0; bx < blur_width; bx++) {
            blur_line(blur + bx, blur_height, blur_width);
        }
    }
}

/* The half size neighbour on the far side of a full size position, staying inside count */
static uint8_t far_neighbour(uint8_t position, uint8_t count) {
    const uint8_t near = position / 2;

    if (position & 1) {
        return near + 1 < count ? near + 1 : near;
    }
    return near ? near - 1 : 0;
}

/* Half a screen row of a blur, scaled up with three parts near to one part far each way, in sixteenths */
typedef struct {
    uint16_t r[MENU_WIDTH];
    uint16_t g[MENU_WIDTH];
    uint16_t b[MENU_WIDTH];
} blur_line_t;

/* Row row of a half size blur scaled up, from full size column x0 */
static void blur_scale_row(const uint16_t *blur, uint8_t blur_width, uint8_t blur_height, uint8_t row, uint8_t x0, blur_line_t *line) {
    const uint16_t *near = blur + (row / 2) * blur_width;
    const uint16_t *far  = blur + far_neighbour(row, blur_height) * blur_width;
    uint16_t        r[BLUR_MAX_SIDE];
    uint16_t        g[BLUR_MAX_SIDE];
    uint16_t        b[BLUR_MAX_SIDE];

    for (uint8_t x = 0; x < blur_width; x++) {
        r[x] = (uint16_t)((near[x] >> 11) * 3 + (far[x] >> 11));
        g[x] = (uint16_t)(((near[x] >> 5) & 0x3F) * 3 + ((far[x] >> 5) & 0x3F));
        b[x] = (uint16_t)((near[x] & 0x1F) * 3 + (far[x] & 0x1F));
    }
    for (uint8_t i = 0; i < MENU_WIDTH; i++) {
        const uint8_t x = x0 + i;
        const uint8_t n = x / 2;
        const uint8_t f = far_neighbour(x, blur_width);
        line->r[i]      = (uint16_t)(r[n] * 3 + r[f]);
        line->g[i]      = (uint16_t)(g[n] * 3 + g[f]);
        line->b[i]      = (uint16_t)(b[n] * 3 + b[f]);
    }
}

/*
 * Screen row y of the blurred page behind the drawer, following the pages as they slide. Behind the
 * tabs only the covered halves are kept, so they slide past each other in step with the strip
 */
static void menu_blur_line(uint8_t y, blur_line_t *line) {
    uint8_t band = band_shown;
    uint8_t row  = y;

    if (page_slide_active && menu_right) {
        blur_line_t   old;
        blur_line_t   next;
        const uint8_t offset = page_slide_drawn;

        blur_scale_row(menu_blur[band], BLUR_WIDTH, BLUR_HEIGHT, y, 0, &old);
        blur_scale_row(menu_blur[band ^ 1], BLUR_WIDTH, BLUR_HEIGHT, y, 0, &next);
        for (uint8_t x = 0; x < MENU_WIDTH; x++) {
            const uint8_t      from   = x + offset;
            const blur_line_t *source = from < MENU_WIDTH ? &old : &next;
            const uint8_t      column = from < MENU_WIDTH ? from : from - MENU_WIDTH;
            line->r[x]                = source->r[column];
            line->g[x]                = source->g[column];
            line->b[x]                = source->b[column];
        }
        return;
    }
    if (page_slide_active) {
        const uint8_t offset = page_slide_drawn;
        if (page_slide_down) {
            if (y < LCD_HEIGHT - offset) {
                row = y + offset;
            } else {
                band ^= 1;
                row = y - (LCD_HEIGHT - offset);
            }
        } else if (y < offset) {
            band ^= 1;
            row = y + LCD_HEIGHT - offset;
        } else {
            row = y - offset;
        }
    }
    blur_scale_row(menu_blur[band], BLUR_WIDTH, BLUR_HEIGHT, row, 0, line);
}

static const uint8_t dither_bayer[4][4] = {{0, 8, 2, 10}, {12, 4, 14, 6}, {3, 11, 1, 9}, {15, 7, 13, 5}};

/* Rounds sixteenths to a channel through a 4x4 ordered dither, so dark gradients do not band */
static uint16_t dither_channel(uint16_t sixteenths, uint8_t threshold, uint16_t max) {
    const uint16_t value = (uint16_t)((sixteenths + threshold) >> 4);
    return value < max ? value : max;
}

/* How much of a foreground pixel is white text, from 0 to FADE_LEVELS */
static uint8_t text_level(uint16_t color) {
    return color == MENU_KEY_CLEAR ? 0 : (uint8_t)((((color >> 5) & 0x3F) * FADE_LEVELS + 31) / 63);
}

/* Rows the highlight covers, moving from the row it left to the selected one as the page slides */
static void menu_bar_span(uint8_t *top, uint8_t *bottom) {
    const uint8_t distance = page_slide_distance();
    const uint8_t progress = page_slide_active ? page_slide_drawn : distance;
    const int16_t from_top = menu_row_top(menu_bar_from);
    const int16_t to_top   = menu_row_top(menu_drawn);
    const int16_t from_end = menu_row_top(menu_bar_from + 1) - 2;
    const int16_t to_end   = menu_row_top(menu_drawn + 1) - 2;

    *top    = (uint8_t)(from_top + (to_top - from_top) * progress / distance);
    *bottom = (uint8_t)(from_end + (to_end - from_end) * progress / distance);
}

/*
 * The drawer showing edge columns from its side: its foreground over the blurred page, darkened,
 * and the highlight over it tinted with the accent, turning the text on it black
 */
static void menu_draw_at(uint8_t edge) {
    const uint16_t tint      = accent_colors[accent];
    /* The tint's share of the glass, in sixteenths of each channel */
    const uint32_t tint_r    = (uint32_t)(tint >> 11) * 16 * MENU_TINT_LEVEL;
    const uint32_t tint_g    = (uint32_t)((tint >> 5) & 0x3F) * 16 * MENU_TINT_LEVEL;
    const uint32_t tint_b    = (uint32_t)(tint & 0x1F) * 16 * MENU_TINT_LEVEL;
    /* Where the drawer's first shown column is on screen, in the drawer, and in the part it covers */
    const uint8_t  screen_x  = menu_right ? LCD_WIDTH - edge : 0;
    const uint8_t  drawer_x  = menu_right ? 0 : MENU_WIDTH - edge;
    const uint8_t  under_x   = menu_right ? MENU_WIDTH - edge : 0;
    const uint8_t  bar_left  = menu_inner_left() + MENU_BAR_MARGIN;
    const uint8_t  bar_right = menu_inner_left() + MENU_WIDTH - 2 - MENU_BAR_MARGIN;
    const uint8_t  marker_x  = menu_inner_left() + MENU_MARKER_X;
    uint8_t        bar_top;
    uint8_t        bar_bottom;
    blur_line_t    blur;

    if (!edge) {
        return;
    }
    menu_bar_span(&bar_top, &bar_bottom);
    const uint8_t marker_y = bar_top + (bar_bottom - bar_top + 2 - font_small.height) / 2;

    lcd_stream_begin(screen_x, 0, screen_x + edge - 1, LAST_Y);
    for (uint8_t y = 0; y < LCD_HEIGHT; y++) {
        const uint8_t *front   = menu_foreground + ((uint32_t)y * MENU_WIDTH + drawer_x) * 2;
        const bool     on_bar  = y >= bar_top && y <= bar_bottom;
        const bool     on_edge = y == bar_top || y == bar_bottom;
        const bool     marked  = y >= marker_y && y < marker_y + font_small.height;

        menu_blur_line(y, &blur);
        for (uint8_t i = 0; i < edge; i++, front += 2) {
            const uint16_t color     = (uint16_t)((front[0] << 8) | front[1]);
            const uint8_t  column    = drawer_x + i;
            const uint8_t  under     = under_x + i;
            const uint8_t  threshold = dither_bayer[y & 3][(screen_x + i) & 3];

            if (on_bar && column >= bar_left && column <= bar_right && !(on_edge && (column == bar_left || column == bar_right))) {
                uint8_t level = text_level(color);
                if (marked && column >= marker_x && column < marker_x + MENU_MARKER_WIDTH) {
                    const uint8_t mark = text_level(pixel_at(menu_marker, (uint32_t)(y - marker_y) * MENU_MARKER_WIDTH + column - marker_x));
                    level              = mark > level ? mark : level;
                }
                const uint16_t r     = dither_channel((uint16_t)((blur.r[under] * (FADE_LEVELS - MENU_TINT_LEVEL) + tint_r) / FADE_LEVELS), threshold, 0x1F);
                const uint16_t g     = dither_channel((uint16_t)((blur.g[under] * (FADE_LEVELS - MENU_TINT_LEVEL) + tint_g) / FADE_LEVELS), threshold, 0x3F);
                const uint16_t b     = dither_channel((uint16_t)((blur.b[under] * (FADE_LEVELS - MENU_TINT_LEVEL) + tint_b) / FADE_LEVELS), threshold, 0x1F);
                const uint16_t glass = (uint16_t)((r << 11) | (g << 5) | b);
                lcd_stream_pixel(level ? mix_color(glass, COLOR_BLACK, level) : glass);
            } else if (color == MENU_KEY_CLEAR) {
                const uint16_t r = dither_channel(blur.r[under] / 2, threshold, 0x1F);
                const uint16_t g = dither_channel(blur.g[under] / 2, threshold, 0x3F);
                const uint16_t b = dither_channel(blur.b[under] / 2, threshold, 0x1F);
                lcd_stream_pixel((uint16_t)((r << 11) | (g << 5) | b));
            } else {
                lcd_stream_pixel(color);
            }
        }
    }
    lcd_stream_end();
}

/* The screen stays black until the keyboard says which mode to show, so no other mode flashes up */
void ui_init(void) {
    start_time = board_millis();
    menu_render_marker();
}

/* In reactive mode the GIF only plays while keys are held or were just pressed */
static bool media_playing(void) {
    return !media_reactive || any_key_held() || board_millis() - media_key_timer < MEDIA_REACTIVE_MS;
}

static void battery_warn_draw(void) {
    const uint8_t right  = BATTERY_WARN_X + BATTERY_WARN_WIDTH - 1;
    const uint8_t bottom = BATTERY_WARN_Y + BATTERY_WARN_HEIGHT - 1;

    draw_fill(BATTERY_WARN_X - 1, 0, right + 3, bottom + 1, COLOR_BLACK);
    draw_outline(BATTERY_WARN_X, BATTERY_WARN_Y, right, bottom, COLOR_LOW);
    draw_fill(right + 1, BATTERY_WARN_Y + 2, right + 2, bottom - 2, COLOR_LOW);
    draw_fill(BATTERY_WARN_X + 2, BATTERY_WARN_Y + 2, BATTERY_WARN_X + 3, bottom - 2, COLOR_LOW);
}

/* The info page already shows the battery */
static void battery_warn_task(bool page_drew) {
    static bool     warning;
    static uint32_t repaint_timer;
    const bool      charging = status_flags & STATUS_FLAG_CHARGING;
    const bool      known    = status_extra_received && status_battery;

    if (!warning && known && !charging && status_battery <= BATTERY_WARN_PERCENT) {
        warning = true;
    } else if (warning && (charging || status_battery >= BATTERY_WARN_CLEAR_PERCENT)) {
        warning = false;
        enter_mode();
        return;
    }

    if (warning && mode != MODE_STATUS && (page_drew || board_millis() - repaint_timer >= BATTERY_WARN_REPAINT_MS)) {
        repaint_timer = board_millis();
        battery_warn_draw();
    }
}

/* Returns true if the media page drew */
static bool page_task(void) {
    bool media_drew = false;

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
        case MODE_CLOCK:
            clock_task();
            break;
        case MODE_MEDIA:
            media_page_task(media_playing());
            media_drew = media_page_take_drawn();
            break;
        default:
            break;
    }
    return media_drew;
}

/* Starts capturing split at the drawer's edge: under is the part the drawer covers, strip the rest */
static void capture_split_begin(uint8_t *under, uint8_t *strip) {
    lcd_capture_begin(menu_right ? strip : under, menu_right ? under : strip, MENU_WIDTH);
}

/* Draws a page from scratch into memory, split at the drawer's edge. Its state then matches what was drawn */
static void capture_page(uint8_t page, uint8_t *under, uint8_t *strip) {
    const uint8_t shown = mode;

    mode = page;
    capture_split_begin(under, strip);
    enter_mode();
    page_task();
    battery_warn_task(true);
    lcd_capture_end();
    mode = shown;
}

/* Draws the view on screen again. Entering the media page again would restart its GIF */
static void render_shown(void) {
    if (mode == MODE_MEDIA) {
        clear_screen();
        media_page_redraw();
        media_page_task(false);
    } else {
        enter_mode();
        page_task();
    }
    battery_warn_task(true);
}

static void menu_reset(void) {
    menu_active        = false;
    menu_edge          = 0;
    page_slide_pending = false;
    page_slide_active  = false;
    fade_active   = false;
}

static void page_slide_draw(uint8_t offset) {
    const uint8_t *old  = page_band(band_shown);
    const uint8_t *next = page_band(band_shown ^ 1);
    const uint16_t row  = MENU_STRIP_WIDTH * 2;

    if (menu_right) {
        /* The new tab comes out from under the drawer */
        if (offset < MENU_STRIP_WIDTH) {
            lcd_blit(0, 0, MENU_STRIP_WIDTH - 1 - offset, LAST_Y, old + offset * 2, MENU_STRIP_WIDTH);
        }
        if (offset) {
            lcd_blit(MENU_STRIP_WIDTH - offset, 0, MENU_STRIP_WIDTH - 1, LAST_Y, next, MENU_STRIP_WIDTH);
        }
    } else if (page_slide_down) {
        if (offset < LCD_HEIGHT) {
            lcd_blit(MENU_WIDTH, 0, LAST_X, LAST_Y - offset, old + offset * row, MENU_STRIP_WIDTH);
        }
        if (offset) {
            lcd_blit(MENU_WIDTH, LCD_HEIGHT - offset, LAST_X, LAST_Y, next, MENU_STRIP_WIDTH);
        }
    } else {
        if (offset) {
            lcd_blit(MENU_WIDTH, 0, LAST_X, offset - 1, next + (LCD_HEIGHT - offset) * row, MENU_STRIP_WIDTH);
        }
        if (offset < LCD_HEIGHT) {
            lcd_blit(MENU_WIDTH, offset, LAST_X, LAST_Y, old, MENU_STRIP_WIDTH);
        }
    }
}

static void strip_draw(uint8_t band) {
    const uint8_t x = menu_right ? 0 : MENU_WIDTH;
    lcd_blit(x, 0, x + MENU_STRIP_WIDTH - 1, LAST_Y, page_band(band), MENU_STRIP_WIDTH);
}

static void page_slide_finish(void) {
    page_slide_active = false;
    band_shown ^= 1;
}

/* Opens a drawer over the page, which the caller has captured into the foreground buffer and band 0 */
static void menu_open(const char *const *names, uint8_t count, uint8_t selected) {
    band_shown  = 0;
    blur_image(menu_foreground, MENU_WIDTH, LCD_HEIGHT, menu_blur[band_shown]);
    menu_names    = names;
    menu_count    = count;
    menu_drawn    = selected;
    menu_bar_from = selected;
    menu_render_foreground();
    menu_active      = true;
    menu_state       = MENU_OPENING;
    menu_edge_from   = 0;
    menu_slide_timer = board_millis();
}

/* The highlight moves to target, and the page behind slides once the drawer is open */
static void menu_move(uint8_t target, bool down) {
    if (menu_state == MENU_CLOSING) {
        menu_state       = MENU_OPENING;
        menu_edge_from   = menu_edge;
        menu_slide_timer = board_millis();
    }
    if (page_slide_active) {
        strip_draw(band_shown ^ 1);
        page_slide_finish();
        menu_draw_at(menu_edge);
    }
    menu_target        = target;
    page_slide_down    = down;
    page_slide_pending = true;
    menu_timer         = board_millis();
}

/*
 * The page behind the page menu moves to mode. Only the shown page is drawn off screen here, for the
 * drawer to open over at once. The new one is drawn when the drawer is open and it slides in
 */
static void menu_change(uint8_t shown, bool down) {
    if (!menu_active) {
        menu_right = false;
        capture_page(shown, menu_foreground, page_band(0));
        menu_open(page_names, MODE_COUNT, shown);
    }
    menu_move(mode, down);
}

/* Opens the tab drawer over the page as it is, before its setting changes */
static void tabs_open(const char *const *names, uint8_t count, uint8_t current) {
    if (menu_active) {
        return;
    }
    menu_right = true;
    capture_split_begin(menu_foreground, page_band(0));
    render_shown();
    lcd_capture_end();
    menu_open(names, count, current);
}

/* Takes the drawer away at once, leaving the page as it is now */
static void menu_dismiss(void) {
    if (!menu_active) {
        return;
    }
    capture_page(mode, page_band(band_shown ^ 1), page_band(band_shown));
    lcd_blit(menu_right ? MENU_WIDTH : 0, 0, (menu_right ? MENU_WIDTH : 0) + MENU_WIDTH - 1, LAST_Y, page_band(band_shown ^ 1), MENU_WIDTH);
    strip_draw(band_shown);
    menu_reset();
}

/* The foreground buffer holds the new page's covered part only until it is blurred */
static void page_slide_begin(void) {
    capture_page(mode, menu_foreground, page_band(band_shown ^ 1));
    blur_image(menu_foreground, MENU_WIDTH, LCD_HEIGHT, menu_blur[band_shown ^ 1]);
    menu_bar_from = menu_drawn;
    menu_drawn    = menu_target;
    menu_render_foreground();
    page_slide_pending = false;
    page_slide_active  = true;
    page_slide_drawn   = 0;
    page_slide_timer   = board_millis();
}

/* Columns of the drawer's covered part newly uncovered as its edge moves from was to edge */
static void menu_uncover(uint8_t was, uint8_t edge) {
    const uint8_t *under = page_band(band_shown ^ 1);

    if (menu_right) {
        const uint8_t from = LCD_WIDTH - was;
        lcd_blit(from, 0, LAST_X - edge, LAST_Y, under + (from - MENU_WIDTH) * 2, MENU_WIDTH);
    } else {
        lcd_blit(edge, 0, was - 1, LAST_Y, under + edge * 2, MENU_WIDTH);
    }
}

static void menu_task(void) {
    const uint32_t now = board_millis();

    switch (menu_state) {
        case MENU_OPENING: {
            const uint8_t edge = menu_edge_from + ease_out(now - menu_slide_timer, MENU_SLIDE_MS, MENU_WIDTH - menu_edge_from);
            if (edge != menu_edge) {
                menu_edge = edge;
                menu_draw_at(edge);
            }
            if (edge == MENU_WIDTH) {
                menu_state = MENU_OPEN;
            }
            break;
        }
        case MENU_OPEN:
            if (page_slide_pending) {
                page_slide_begin();
            }
            if (page_slide_active) {
                const uint8_t distance = page_slide_distance();
                const uint8_t offset   = ease_out(board_millis() - page_slide_timer, PAGE_SLIDE_MS, distance);
                if (offset != page_slide_drawn) {
                    page_slide_drawn = offset;
                    page_slide_draw(offset);
                    menu_draw_at(MENU_WIDTH);
                }
                if (offset == distance) {
                    page_slide_finish();
                }
            } else if (now - menu_timer >= MENU_SHOW_MS) {
                /* The whole page afresh, as the strip on screen was drawn when the selection moved */
                capture_page(mode, page_band(band_shown ^ 1), page_band(band_shown));
                blur_image(page_band(band_shown ^ 1), MENU_WIDTH, LCD_HEIGHT, menu_blur[band_shown]);
                strip_draw(band_shown);
                menu_draw_at(menu_edge);
                menu_state       = MENU_CLOSING;
                menu_edge_from   = menu_edge;
                menu_slide_timer = board_millis();
            }
            break;
        case MENU_CLOSING: {
            const uint8_t edge = menu_edge_from - ease_out(now - menu_slide_timer, MENU_SLIDE_MS, menu_edge_from);
            if (edge != menu_edge) {
                menu_uncover(menu_edge, edge);
                menu_edge = edge;
                menu_draw_at(edge);
            }
            if (!edge) {
                menu_reset();
            }
            break;
        }
    }
}

/* Captures the view on screen whole, as what a fade or popup starts from, finishing any fade still running */
static void fade_prepare(void) {
    if (fade_active) {
        lcd_blit(0, 0, LAST_X, LAST_Y, gif_scratch(), LCD_WIDTH);
        fade_active = false;
    }
    lcd_capture_begin(page_frame, 0, LCD_WIDTH);
    render_shown();
    lcd_capture_end();
}

/* Captures the page as it now is into the GIF canvas, which no GIF is using, as what a fade ends on */
static void fade_capture_page(void) {
    lcd_capture_begin(gif_scratch(), 0, LCD_WIDTH);
    enter_mode();
    page_task();
    battery_warn_task(true);
    lcd_capture_end();
}

/* Fades from the captured old view to the new one, over the rows where they differ */
static void fade_start(void) {
    const uint8_t *next   = gif_scratch();
    uint8_t        top    = LCD_HEIGHT;
    uint8_t        bottom = 0;

    for (uint8_t y = 0; y < LCD_HEIGHT; y++) {
        if (memcmp(page_frame + y * LCD_WIDTH * 2, next + y * LCD_WIDTH * 2, LCD_WIDTH * 2) != 0) {
            top    = top < y ? top : y;
            bottom = y;
        }
    }
    if (top > bottom) {
        return;
    }
    fade_active = true;
    fade_top    = top;
    fade_bottom = bottom;
    fade_drawn  = 0;
    fade_timer  = board_millis();
}

static void fade_task(void) {
    const uint8_t level = ease_out(board_millis() - fade_timer, ACCENT_FADE_MS, FADE_LEVELS);

    if (level != fade_drawn) {
        fade_drawn = level;
        lcd_stream_begin(0, fade_top, LAST_X, fade_bottom);
        for (uint32_t i = (uint32_t)fade_top * LCD_WIDTH; i < (uint32_t)(fade_bottom + 1) * LCD_WIDTH; i++) {
            const uint16_t from = pixel_at(page_frame, i);
            const uint16_t to   = pixel_at(gif_scratch(), i);
            lcd_stream_pixel(from == to ? to : mix_color(from, to, level));
        }
        lcd_stream_end();
    }
    if (level == FADE_LEVELS) {
        fade_active = false;
    }
}

/* The menu's blurs are free while a popup shows, and together hold the popup's whole screen blur */
_Static_assert(sizeof(menu_blur) >= POPUP_BLUR_SIDE * POPUP_BLUR_SIDE * 2, "the popup's blur fits in the menu's");
#define popup_blur ((uint16_t *)menu_blur)

static bool is_grey(uint16_t color) {
    const int16_t r = color >> 11;
    const int16_t g = (color >> 5) & 0x3F;
    const int16_t b = color & 0x1F;
    return r == b && (g / 2 - r) <= 1 && (r - g / 2) <= 1;
}

/* The popup page drawn into half the screen's columns, over the blur, into the finished popup */
static void popup_render_half(uint8_t half, uint8_t *finished) {
    const uint8_t x0    = half * MENU_WIDTH;
    const uint8_t shown = mode;
    blur_line_t   blur;

    mode = popup_mode;
    lcd_capture_begin(half ? 0 : menu_foreground, half ? menu_foreground : 0, MENU_WIDTH);
    enter_mode();
    page_task();
    lcd_capture_end();
    mode = shown;

    for (uint8_t y = 0; y < LCD_HEIGHT; y++) {
        uint8_t *out = finished + ((uint32_t)y * LCD_WIDTH + x0) * 2;

        blur_scale_row(popup_blur, POPUP_BLUR_SIDE, POPUP_BLUR_SIDE, y, x0, &blur);
        for (uint8_t i = 0; i < MENU_WIDTH; i++, out += 2) {
            const uint16_t front     = pixel_at(menu_foreground, (uint32_t)y * MENU_WIDTH + i);
            const uint8_t  threshold = dither_bayer[y & 3][(x0 + i) & 3];
            const uint16_t r         = dither_channel(blur.r[i] / 2, threshold, 0x1F);
            const uint16_t g         = dither_channel(blur.g[i] / 2, threshold, 0x3F);
            const uint16_t b         = dither_channel(blur.b[i] / 2, threshold, 0x1F);
            const uint16_t glass     = (uint16_t)((r << 11) | (g << 5) | b);
            uint16_t       color     = front;

            if (front == COLOR_BLACK) {
                color = glass;
            } else if (is_grey(front)) {
                color = mix_color(glass, COLOR_WHITE, (uint8_t)((((front >> 5) & 0x3F) * FADE_LEVELS + 31) / 63));
            }
            out[0] = (uint8_t)(color >> 8);
            out[1] = (uint8_t)color;
        }
    }
}

/*
 * The finished popup goes in the GIF canvas, so fading only has to blend it with the page under it.
 * A GIF on that page gets its frame back from the page when the popup ends
 */
static void popup_render(void) {
    popup_render_half(0, gif_scratch());
    popup_render_half(1, gif_scratch());
}

static void popup_draw(uint8_t level) {
    const uint8_t *finished = gif_scratch();

    if (!level) {
        lcd_blit(0, 0, LAST_X, LAST_Y, page_frame, LCD_WIDTH);
    } else if (level == FADE_LEVELS) {
        lcd_blit(0, 0, LAST_X, LAST_Y, finished, LCD_WIDTH);
    } else {
        lcd_stream_begin(0, 0, LAST_X, LAST_Y);
        for (uint32_t i = 0; i < (uint32_t)LCD_WIDTH * LCD_HEIGHT; i++) {
            lcd_stream_pixel(mix_color(pixel_at(page_frame, i), pixel_at(finished, i), level));
        }
        lcd_stream_end();
    }
}

/* The view on screen is kept whole for fading back to, and blurred for the glass */
static void popup_begin(uint8_t page) {
    fade_prepare();
    /* Capturing it entered the page again, which is not a reason for the cat to greet */
    intro_taps = 0;
    blur_image(page_frame, LCD_WIDTH, LCD_HEIGHT, popup_blur);
    popup_active     = true;
    popup_mode       = page;
    popup_state      = POPUP_FADING_IN;
    popup_level      = 0;
    popup_level_from = 0;
    popup_render();
    popup_dirty      = false;
    popup_fade_timer = board_millis();
}

/* Puts back the GIF frame that the finished popup drew over */
static void popup_finish(void) {
    uint8_t x, y, width, height;

    popup_active = false;
    if (mode == MODE_MEDIA && media_page_gif_rect(&x, &y, &width, &height)) {
        uint8_t *canvas = gif_scratch();
        for (uint8_t row = 0; row < height; row++) {
            memcpy(canvas + (uint32_t)row * width * 2, page_frame + ((uint32_t)(y + row) * LCD_WIDTH + x) * 2, width * 2u);
        }
    }
}

/* Ends a popup at once, leaving the view under it */
static void popup_end(void) {
    if (popup_active) {
        lcd_blit(0, 0, LAST_X, LAST_Y, page_frame, LCD_WIDTH);
        popup_finish();
    }
}

/* Evenly paced, so the fade out shows as much as the fade in */
static void popup_fade_to(uint8_t target) {
    const uint32_t elapsed  = board_millis() - popup_fade_timer;
    const uint8_t  distance = target > popup_level_from ? target - popup_level_from : popup_level_from - target;
    const uint8_t  moved    = elapsed >= POPUP_FADE_MS ? distance : (uint8_t)(elapsed * distance / POPUP_FADE_MS);
    const uint8_t  level    = target > popup_level_from ? popup_level_from + moved : popup_level_from - moved;

    if (popup_dirty) {
        popup_render();
        popup_dirty = false;
        popup_level = level;
        popup_draw(level);
    } else if (level != popup_level) {
        popup_level = level;
        popup_draw(level);
    }
}

static void popup_task(void) {
    switch (popup_state) {
        case POPUP_FADING_IN:
            popup_fade_to(FADE_LEVELS);
            if (popup_level == FADE_LEVELS) {
                popup_state = POPUP_SHOWN;
            }
            break;
        case POPUP_SHOWN:
            if (board_millis() - popup_timer >= PEEK_SHOW_MS) {
                popup_state      = POPUP_FADING_OUT;
                popup_level_from = popup_level;
                popup_fade_timer = board_millis();
            } else if (popup_dirty) {
                popup_render();
                popup_dirty = false;
                popup_draw(popup_level);
            }
            break;
        case POPUP_FADING_OUT:
            popup_fade_to(0);
            if (!popup_level) {
                popup_finish();
            }
            break;
    }
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
        menu_task();
        return;
    }
    if (popup_active) {
        popup_task();
        return;
    }
    if (fade_active) {
        fade_task();
        return;
    }

    battery_warn_task(page_task());
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
    popup_dirty    = popup_dirty || (popup_active && popup_mode == MODE_RGB);
}

void ui_on_rgb_name(const char *name) {
    for (uint8_t i = 0; i < RGB_NAME_SIZE; i++) {
        if (rgb_name[i] != name[i]) {
            rgb_dirty   = true;
            popup_dirty = popup_dirty || (popup_active && popup_mode == MODE_RGB);
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
    popup_active   = false;
    menu_reset();
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
    if (powered && mode == MODE_MEDIA && !waiting_for_mode && !menu_active && !popup_active && !media_upload_active()) {
        enter_mode();
    }
}

void ui_on_media(uint8_t setting) {
    if (setting >= MEDIA_SETTING_COUNT || setting == media_setting) {
        return;
    }
    /* The page under a popup would be left showing the old setting */
    popup_end();
    const bool visible = powered && mode == MODE_MEDIA && !waiting_for_mode && (!menu_active || menu_right) && !media_upload_active();

    if (visible) {
        tabs_open(media_setting_names, MEDIA_SETTING_COUNT, media_setting);
    }
    media_setting  = setting;
    media_reactive = setting == MEDIA_SETTING_REACTIVE;
    media_show_gif = setting != MEDIA_SETTING_IMAGE;
    if (visible) {
        menu_move(setting, true);
    }
}

void ui_on_mode(uint8_t value) {
    const uint8_t shown = mode;

    if (value >= MODE_COUNT || (value == mode && !waiting_for_mode)) {
        return;
    }
    /* The menu opens over the page itself */
    popup_end();
    if (menu_active && menu_right) {
        menu_dismiss();
    }
    const bool first_mode = waiting_for_mode;
    /* Wrapping around counts as a step, as the keyboard only steps one page at a time */
    const bool down = value == (mode + 1) % MODE_COUNT || (value != (mode + MODE_COUNT - 1) % MODE_COUNT && value > mode);

    mode             = value;
    waiting_for_mode = false;
    if (!powered || media_upload_active()) {
        return;
    }
    if (first_mode) {
        enter_mode();
    } else {
        menu_change(shown, down);
    }
}

void ui_on_peek(uint8_t value) {
    if (value >= MODE_COUNT || waiting_for_mode || !powered || menu_active || media_upload_active()) {
        return;
    }
    if (!popup_active) {
        if (value == mode) {
            return;
        }
        popup_begin(value);
    } else if (popup_state == POPUP_FADING_OUT) {
        popup_state      = POPUP_FADING_IN;
        popup_level_from = popup_level;
        popup_fade_timer = board_millis();
    }
    popup_timer = board_millis();
}

void ui_on_power(bool on) {
    if (on == powered) {
        return;
    }
    powered      = on;
    popup_active = false;
    menu_reset();
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
    if (powered && (menu_active || popup_active || fade_active)) {
        return true;
    }
    return powered && mode == MODE_MEDIA && media_show_gif && !waiting_for_mode && media_playing();
}

bool ui_can_sleep(void) {
    return !powered && !media_upload_active();
}

void ui_on_accent(uint8_t value) {
    if (value >= ACCENT_COUNT || value == accent) {
        return;
    }
    popup_end();
    /* The media page keeps a GIF in the canvas that the new side would be drawn into */
    const bool visible = powered && mode != MODE_MEDIA && !waiting_for_mode && !menu_active && !media_upload_active();

    if (visible) {
        fade_prepare();
    }
    accent = value;
    if (visible) {
        fade_capture_page();
        /* A new color is not a new page, so the cat skips its greeting taps */
        intro_taps = 0;
        fade_start();
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
    if (mode == MODE_CLOCK && powered && !waiting_for_mode && !menu_active && !popup_active && !media_upload_active()) {
        clock_enter();
    }
}

void ui_on_clock_face(uint8_t value) {
    if (value >= CLOCK_FACE_COUNT || value == clock_face) {
        return;
    }
    popup_end();
    const bool visible = mode == MODE_CLOCK && powered && !waiting_for_mode && (!menu_active || menu_right) && !media_upload_active();

    if (visible) {
        tabs_open(clock_face_names, CLOCK_FACE_COUNT, clock_face);
    }
    clock_face = value;
    if (visible) {
        menu_move(value, true);
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
