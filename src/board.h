#pragma once

#include <stdbool.h>
#include <stdint.h>

void     board_init(void);
uint32_t board_millis(void);
void     board_delay_ms(uint32_t ms);
void     board_reset(void);

/* Runs the CPU at 144 MHz instead of 72 MHz, for when drawing can't keep up */
void     board_set_fast(bool fast);

/*
 * Stops the clocks until a falling edge on the UART RX pin. SRAM and pin states are kept, and
 * board_millis() counts on using the RTC. Returns false without sleeping if the line is busy.
 */
bool     board_deep_sleep(void);

/* For diagnostics: the RTC clock source, 0 if there is none, and how many times it has slept */
uint8_t  board_rtc_source(void);
uint16_t board_sleep_count(void);
