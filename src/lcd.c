#include "at32f403a_407.h"
#include "board.h"
#include "lcd.h"
#include "panel.h"

/* The controller addresses 128x128. The panel is mounted rotated, which MADCTL_ROTATED undoes, so screen (x, y) is column x, row 10 + y */
#define CONTROLLER_SIZE 128
#define OFFSET_ROW 10
#define MADCTL_ROTATED 0x60

#define PIN_RS GPIO_PINS_6  /* GPIOA */
#define PIN_BLK GPIO_PINS_3 /* GPIOA */
#define PIN_RES GPIO_PINS_0 /* GPIOB */
#define PIN_CS GPIO_PINS_2  /* GPIOB */

/* From the 72 MHz timer clock, 20 kHz is above what the eye or ear can pick up */
#define PWM_PERIOD 3600u
#define BRIGHTNESS_FULL 100u

/* Fixed to SPI1 TX by the hardware */
#define SPI_DMA DMA1_CHANNEL3

static bool    panel_awake  = true;
static bool    backlight_on = false;
static uint8_t brightness   = BRIGHTNESS_FULL;
static bool    pwm_running;
static bool    dma_running;
/* Command, data length, data */
static const uint8_t init_table[] = {
    0xFE, 0,
    0xEF, 0,
    0xB0, 1, 0xC0,
    0xB1, 1, 0x80,
    0xB2, 1, 0x27,
    0xB3, 1, 0x13,
    0xB6, 1, 0x19,
    0xB7, 1, 0x05,
    0xAC, 1, 0xC8,
    0xAB, 1, 0x0F,
    0x3A, 1, 0x05,
    0xB4, 1, 0x04,
    0xA8, 1, 0x08,
    0xB8, 1, 0x08,
    0xEA, 1, 0x02,
    0xE8, 1, 0x2A,
    0xE9, 1, 0x47,
    0xE7, 1, 0x5F,
    0xC6, 1, 0x21,
    0xC7, 1, 0x15,
    0xF0, 14, 0x1D, 0x38, 0x09, 0x4D, 0x92, 0x2F, 0x35, 0x52, 0x1E, 0x0C, 0x04, 0x12, 0x14, 0x1F,
    0xF1, 14, 0x16, 0x40, 0x1C, 0x54, 0xA9, 0x2D, 0x2E, 0x56, 0x10, 0x0D, 0x0C, 0x1A, 0x14, 0x1E,
    0xF4, 3, 0x00, 0x00, 0xFF,
    0xBA, 2, 0xFF, 0xFF,
    0x36, 1, MADCTL_ROTATED,
};

static inline void spi_send(uint8_t b) {
    while (!SPI1->sts_bit.tdbe) {
    }
    SPI1->dt = b;
}

static void spi_wait_idle(void) {
    while (!SPI1->sts_bit.tdbe) {
    }
    while (SPI1->sts_bit.bf) {
    }
}

static void select_command(void) {
    lcd_wait_idle();
    GPIOA->clr = PIN_RS;
    GPIOB->clr = PIN_CS;
}

static void select_data(void) {
    spi_wait_idle();
    GPIOA->scr = PIN_RS;
}

static void deselect(void) {
    spi_wait_idle();
    GPIOB->scr = PIN_CS;
}

void lcd_wait_idle(void) {
    if (!dma_running) {
        return;
    }
    while (!(DMA1->sts & (DMA1_FDT3_FLAG | DMA1_DTERR3_FLAG))) {
    }
    dma_channel_enable(SPI_DMA, FALSE);
    dma_flag_clear(DMA1_GL3_FLAG);
    dma_running = false;
    deselect();
}

static void lcd_write(uint8_t cmd, const uint8_t *data, uint32_t len) {
    select_command();
    spi_send(cmd);
    select_data();
    while (len--) {
        spi_send(*data++);
    }
    deselect();
}

static void pins_init(void) {
    gpio_init_type gpio;

    crm_periph_clock_enable(CRM_GPIOA_PERIPH_CLOCK, TRUE);
    crm_periph_clock_enable(CRM_GPIOB_PERIPH_CLOCK, TRUE);
    crm_periph_clock_enable(CRM_SPI1_PERIPH_CLOCK, TRUE);
    crm_periph_clock_enable(CRM_DMA1_PERIPH_CLOCK, TRUE);

    gpio_default_para_init(&gpio);
    gpio.gpio_drive_strength = GPIO_DRIVE_STRENGTH_STRONGER;
    gpio.gpio_out_type       = GPIO_OUTPUT_PUSH_PULL;

    gpio.gpio_mode = GPIO_MODE_OUTPUT;
    gpio.gpio_pins = PIN_RS | PIN_BLK;
    gpio_init(GPIOA, &gpio);
    gpio.gpio_pins = PIN_RES | PIN_CS;
    gpio_init(GPIOB, &gpio);

    gpio.gpio_mode = GPIO_MODE_MUX;
    gpio.gpio_pins = GPIO_PINS_5 | GPIO_PINS_7;
    gpio_init(GPIOA, &gpio);

    GPIOB->scr = PIN_RES | PIN_CS;
    GPIOA->scr = PIN_RS;
    GPIOA->clr = PIN_BLK;
}

static void spi_setup(void) {
    spi_init_type spi;

    spi_default_para_init(&spi);
    spi.transmission_mode      = SPI_TRANSMIT_FULL_DUPLEX;
    spi.master_slave_mode      = SPI_MODE_MASTER;
    spi.mclk_freq_division     = SPI_MCLK_DIV_2;
    spi.first_bit_transmission = SPI_FIRST_BIT_MSB;
    spi.frame_bit_num          = SPI_FRAME_8BIT;
    spi.clock_polarity         = SPI_CLOCK_POLARITY_HIGH;
    spi.clock_phase            = SPI_CLOCK_PHASE_2EDGE;
    spi.cs_mode_selection      = SPI_CS_SOFTWARE_MODE;
    spi_init(SPI1, &spi);
    spi_i2s_dma_transmitter_enable(SPI1, TRUE);
    spi_enable(SPI1, TRUE);
}

static void dma_setup(void) {
    dma_init_type dma;

    dma_reset(SPI_DMA);
    dma_default_para_init(&dma);
    dma.peripheral_base_addr  = (uint32_t)&SPI1->dt;
    dma.direction             = DMA_DIR_MEMORY_TO_PERIPHERAL;
    dma.peripheral_inc_enable = FALSE;
    dma.memory_inc_enable     = TRUE;
    dma.peripheral_data_width = DMA_PERIPHERAL_DATA_WIDTH_BYTE;
    dma.memory_data_width     = DMA_MEMORY_DATA_WIDTH_BYTE;
    dma.loop_mode_enable      = FALSE;
    dma.priority              = DMA_PRIORITY_MEDIUM;
    dma_init(SPI_DMA, &dma);
}

static void clear_memory(void) {
    const uint8_t window[4] = {0, 0, 0, CONTROLLER_SIZE - 1};

    lcd_write(0x2A, window, sizeof(window));
    lcd_write(0x2B, window, sizeof(window));
    select_command();
    spi_send(0x2C);
    select_data();
    for (uint32_t i = 0; i < CONTROLLER_SIZE * CONTROLLER_SIZE * 2; i++) {
        spi_send(0);
    }
    deselect();
}

void lcd_init(void) {
    pins_init();
    spi_setup();
    dma_setup();

    GPIOB->scr = PIN_RES;
    board_delay_ms(10);
    GPIOB->clr = PIN_RES;
    board_delay_ms(10);
    GPIOB->scr = PIN_RES;
    board_delay_ms(120);

    for (uint32_t i = 0; i < sizeof(init_table);) {
        uint8_t cmd = init_table[i++];
        uint8_t len = init_table[i++];
        lcd_write(cmd, &init_table[i], len);
        i += len;
    }

    lcd_write(0x11, 0, 0);
    board_delay_ms(120);
    clear_memory();
    lcd_write(0x29, 0, 0);
    lcd_set_backlight(true);
}

static void blk_set_mode(gpio_mode_type mode) {
    gpio_init_type gpio;

    gpio_default_para_init(&gpio);
    gpio.gpio_drive_strength = GPIO_DRIVE_STRENGTH_STRONGER;
    gpio.gpio_out_type       = GPIO_OUTPUT_PUSH_PULL;
    gpio.gpio_mode           = mode;
    gpio.gpio_pins           = PIN_BLK;
    gpio_init(GPIOA, &gpio);
}

static void pwm_start(void) {
    tmr_output_config_type output;

    crm_periph_clock_enable(CRM_TMR2_PERIPH_CLOCK, TRUE);
    tmr_base_init(TMR2, PWM_PERIOD - 1, 0);
    tmr_cnt_dir_set(TMR2, TMR_COUNT_UP);
    tmr_output_default_para_init(&output);
    output.oc_mode         = TMR_OUTPUT_CONTROL_PWM_MODE_A;
    output.oc_polarity     = TMR_OUTPUT_ACTIVE_HIGH;
    output.oc_output_state = TRUE;
    tmr_output_channel_config(TMR2, TMR_SELECT_CHANNEL_4, &output);
    tmr_output_channel_buffer_enable(TMR2, TMR_SELECT_CHANNEL_4, TRUE);
    tmr_counter_enable(TMR2, TRUE);
    blk_set_mode(GPIO_MODE_MUX);
    pwm_running = true;
}

/* Leaves the pin holding a level, with the timer off, so it stays that way in deep sleep */
static void pwm_stop(bool level) {
    if (level) {
        GPIOA->scr = PIN_BLK;
    } else {
        GPIOA->clr = PIN_BLK;
    }
    if (pwm_running) {
        blk_set_mode(GPIO_MODE_OUTPUT);
        tmr_counter_enable(TMR2, FALSE);
        crm_periph_clock_enable(CRM_TMR2_PERIPH_CLOCK, FALSE);
        pwm_running = false;
    }
}

static void backlight_apply(void) {
    if (!panel_awake || !backlight_on || brightness == 0) {
        pwm_stop(false);
    } else if (brightness >= BRIGHTNESS_FULL) {
        pwm_stop(true);
    } else {
        if (!pwm_running) {
            pwm_start();
        }
        /* Squared, as perceived brightness is far from linear in duty */
        tmr_channel_value_set(TMR2, TMR_SELECT_CHANNEL_4, PWM_PERIOD * brightness * brightness / (BRIGHTNESS_FULL * BRIGHTNESS_FULL));
    }
}

void lcd_set_backlight(bool on) {
    backlight_on = on;
    backlight_apply();
}

void lcd_set_brightness(uint8_t percent) {
    brightness = percent;
    backlight_apply();
}

void lcd_set_power(bool on) {
    if (on) {
        lcd_write(0x11, 0, 0);
        board_delay_ms(120);
        lcd_write(0x29, 0, 0);
        panel_awake = true;
        backlight_apply();
    } else {
        panel_awake = false;
        backlight_apply();
        lcd_write(0x28, 0, 0);
        lcd_write(0x10, 0, 0);
    }
}

void panel_window(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1) {
    const uint8_t columns[4] = {0, x0, 0, x1};
    const uint8_t rows[4]    = {0, (uint8_t)(OFFSET_ROW + y0), 0, (uint8_t)(OFFSET_ROW + y1)};

    lcd_write(0x2A, columns, sizeof(columns));
    lcd_write(0x2B, rows, sizeof(rows));
    select_command();
    spi_send(0x2C);
    select_data();
}

void panel_pixel(uint16_t color) {
    spi_send((uint8_t)(color >> 8));
    spi_send((uint8_t)color);
}

void panel_write(const uint8_t *bytes, uint16_t len) {
    while (len--) {
        spi_send(*bytes++);
    }
}

void panel_end(void) {
    deselect();
}

void panel_send_rect(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, const uint8_t *pixels) {
    panel_window(x0, y0, x1, y1);
    SPI_DMA->maddr = (uint32_t)pixels;
    dma_data_number_set(SPI_DMA, (uint16_t)((x1 - x0 + 1u) * (y1 - y0 + 1u) * 2u));
    dma_running = true;
    dma_channel_enable(SPI_DMA, TRUE);
}
