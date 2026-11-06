/* Just enough of the Artery SDK for the firmware's sources to build on a PC */
#pragma once

#include <stdint.h>

typedef enum { FALSE, TRUE } confirm_state;
typedef enum { FLASH_OPERATE_DONE } flash_status_type;

typedef struct {
    uint32_t gpio_pins, gpio_out_type, gpio_pull, gpio_mode, gpio_drive_strength;
} gpio_init_type;

enum { CRM_IOMUX_PERIPH_CLOCK, CRM_GPIOA_PERIPH_CLOCK, CRM_GPIOB_PERIPH_CLOCK };
enum { GPIO_DRIVE_STRENGTH_STRONGER, GPIO_OUTPUT_PUSH_PULL, GPIO_MODE_MUX };
enum { GPIOA, GPIOB };

#define GPIO_PINS_1 (1u << 1)
#define GPIO_PINS_6 (1u << 6)
#define GPIO_PINS_7 (1u << 7)
#define GPIO_PINS_8 (1u << 8)
#define GPIO_PINS_10 (1u << 10)
#define GPIO_PINS_11 (1u << 11)

#define crm_periph_clock_enable(clock, state) ((void)0)
#define gpio_default_para_init(init) ((void)0)
#define gpio_init(port, init) ((void)0)

#define flash_spim_unlock() ((void)0)
#define flash_spim_lock() ((void)0)
#define flash_sector_erase(address) FLASH_OPERATE_DONE
#define flash_word_program(address, value) FLASH_OPERATE_DONE
