#pragma once

#include <stdbool.h>
#include <stdint.h>

void ui_init(void);
void ui_task(void);

void ui_on_tap(void);
void ui_on_key_tap(uint8_t row, uint8_t col);
void ui_on_wpm(uint16_t wpm);
void ui_on_status(uint8_t device, uint8_t layer, bool caps, uint8_t battery);
void ui_on_keys(uint8_t row, uint16_t state);
void ui_on_mode(uint8_t mode);
void ui_on_power(bool on);
void ui_on_backlight(bool on);
void ui_on_brightness(uint8_t percent);

/* The panel is off and nothing needs the MCU running */
bool ui_can_sleep(void);
void ui_on_layout(bool iso);
void ui_on_accent(uint8_t accent);
void ui_on_units(uint8_t units);
void ui_on_time(uint8_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second);
void ui_on_clock_face(uint8_t face);
void ui_on_status_extra(uint8_t flags);
void ui_on_rgb(bool enabled, uint8_t mode, uint8_t hue, uint8_t saturation, uint8_t value, uint8_t speed);
void ui_on_rgb_name(const char *name);
void ui_on_build(const char *build);
void ui_on_media(uint8_t setting);
void ui_upload_begin(bool gif);
void ui_upload_progress(uint8_t percent);
void ui_upload_end(void);
void ui_media_changed(void);
void ui_on_versions(const char *qmk, uint16_t via);
void ui_on_system(uint8_t cpu, uint8_t gpu, uint16_t download);
void ui_on_weather(uint8_t icon, uint8_t current, uint8_t low, uint8_t high);
