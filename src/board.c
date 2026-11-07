#include "at32f403a_407.h"
#include "board.h"
#include "lcd.h"
#include "link.h"

/* The stock firmware's RTC period, kept so that it finds the RTC as it left it */
#define RTC_PERIOD_MS 1000u
#define LICK_1HZ_DIVIDER 39999u
/* Enough RTC ticks of awake time to trust the measured rate over the nominal one */
#define RTC_CALIBRATION_TICKS 60u
#define RTC_WAIT_LOOPS 200000u
/* The clock needs up to 120 us after a wake, at 8 MHz */
#define WAKE_SETTLE_LOOPS 400u
#define RX_WAKE_LINE EXINT_LINE_10

extern const uint32_t g_pfnVectors[];

static volatile uint32_t ticks;

static uint8_t  rtc_source;
static uint16_t sleep_count;
static uint32_t awake_ms;
static uint32_t awake_rtc;
static uint32_t mark_ms;
static uint32_t mark_rtc;

void SysTick_Handler(void) {
    ticks++;
}

void EXINT15_10_IRQHandler(void) {
    exint_flag_clear(RX_WAKE_LINE);
}

static void hext_start(void) {
    crm_clock_source_enable(CRM_CLOCK_SOURCE_HEXT, TRUE);
    while (crm_hext_stable_wait() == ERROR) {
    }
}

static void pll_start(void) {
    crm_clock_source_enable(CRM_CLOCK_SOURCE_PLL, TRUE);
    while (crm_flag_get(CRM_PLL_STABLE_FLAG) != SET) {
    }
}

static void sysclk_to_pll(void) {
    crm_auto_step_mode_enable(TRUE);
    crm_sysclk_switch(CRM_SCLK_PLL);
    while (crm_sysclk_switch_status_get() != CRM_SCLK_PLL) {
    }
    crm_auto_step_mode_enable(FALSE);
}

/*
 * 8 MHz HEXT / 2 * 36 = 144 MHz, then 72 MHz through the AHB divider, well below the 240 MHz stock
 * runs at. APB2 stays at 72 MHz and APB1 at 36 MHz at either speed, so the peripherals never notice.
 */
static void clock_init(void) {
    crm_reset();
    hext_start();
    crm_pll_config(CRM_PLL_SOURCE_HEXT_DIV, CRM_PLL_MULT_36, CRM_PLL_OUTPUT_RANGE_GT72MHZ);
    crm_hext_clock_div_set(CRM_HEXT_DIV_2);
    pll_start();
    crm_ahb_div_set(CRM_AHB_DIV_2);
    crm_apb2_div_set(CRM_APB2_DIV_1);
    crm_apb1_div_set(CRM_APB1_DIV_2);
    sysclk_to_pll();
    system_core_clock_update();
}

/* Waits for the RTC registers to be readable. Without a working RTC clock this would never finish */
static bool rtc_sync(void) {
    rtc_flag_clear(RTC_UPDF_FLAG);
    for (uint32_t i = 0; i < RTC_WAIT_LOOPS; i++) {
        if (rtc_flag_get(RTC_UPDF_FLAG) == SET) {
            return true;
        }
    }
    return false;
}

static bool lick_start(void) {
    crm_clock_source_enable(CRM_CLOCK_SOURCE_LICK, TRUE);
    for (uint32_t i = 0; i < RTC_WAIT_LOOPS; i++) {
        if (crm_flag_get(CRM_LICK_STABLE_FLAG) == SET) {
            return true;
        }
    }
    return false;
}

/* Keeps a source that runs in deep sleep and which stock already set up, or starts LICK */
static void rtc_init(void) {
    crm_periph_clock_enable(CRM_PWC_PERIPH_CLOCK, TRUE);
    crm_periph_clock_enable(CRM_BPR_PERIPH_CLOCK, TRUE);
    pwc_battery_powered_domain_access(TRUE);

    const uint32_t source   = CRM->bpdc_bit.rtcsel;
    const bool     has_lext = source == CRM_RTC_CLOCK_LEXT && CRM->bpdc_bit.lextstbl;
    const bool     has_lick = source == CRM_RTC_CLOCK_LICK && lick_start();

    if (!CRM->bpdc_bit.rtcen || (!has_lext && !has_lick)) {
        bpr_reset();
        if (!lick_start()) {
            return;
        }
        crm_rtc_clock_select(CRM_RTC_CLOCK_LICK);
        crm_rtc_clock_enable(TRUE);
        if (!rtc_sync()) {
            return;
        }
        rtc_wait_config_finish();
        rtc_divider_set(LICK_1HZ_DIVIDER);
        rtc_wait_config_finish();
    }
    if (rtc_sync()) {
        rtc_source = (uint8_t)CRM->bpdc_bit.rtcsel;
        mark_rtc   = rtc_counter_get();
    }
}

static uint32_t rtc_ticks_to_ms(uint32_t rtc_ticks) {
    if (awake_rtc < RTC_CALIBRATION_TICKS) {
        return rtc_ticks * RTC_PERIOD_MS;
    }
    return (uint32_t)((uint64_t)rtc_ticks * awake_ms / awake_rtc);
}

bool board_deep_sleep(void) {
    uint32_t rtc_before = 0;

    __disable_irq();

    /* Armed before the check, so anything that starts arriving after it ends the sleep at once */
    exint_flag_clear(RX_WAKE_LINE);
    EXINT->inten |= RX_WAKE_LINE;
    if (!link_rx_idle()) {
        EXINT->inten &= ~RX_WAKE_LINE;
        __enable_irq();
        return false;
    }

    if (rtc_source) {
        rtc_before = rtc_counter_get();
        awake_rtc += rtc_before - mark_rtc;
        awake_ms += ticks - mark_ms;
    }

    SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
    SCB->ICSR = SCB_ICSR_PENDSTCLR_Msk;
    pwc_voltage_regulate_set(PWC_REGULATOR_LOW_POWER);
    pwc_deep_sleep_mode_enter(PWC_DEEP_SLEEP_ENTER_WFI);

    for (volatile uint32_t i = 0; i < WAKE_SETTLE_LOOPS; i++) {
    }
    hext_start();
    pll_start();
    sysclk_to_pll();
    SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;

    if (rtc_source && rtc_sync()) {
        const uint32_t rtc_after = rtc_counter_get();
        ticks += rtc_ticks_to_ms(rtc_after - rtc_before);
        mark_rtc = rtc_after;
    }
    mark_ms = ticks;
    sleep_count++;

    EXINT->inten &= ~RX_WAKE_LINE;
    __enable_irq();
    return true;
}

uint8_t board_rtc_source(void) {
    return rtc_source;
}

uint16_t board_sleep_count(void) {
    return sleep_count;
}

static void rx_wake_init(void) {
    exint_init_type exint;

    crm_periph_clock_enable(CRM_IOMUX_PERIPH_CLOCK, TRUE);
    gpio_exint_line_config(GPIO_PORT_SOURCE_GPIOA, GPIO_PINS_SOURCE10);
    exint_default_para_init(&exint);
    exint.line_mode     = EXINT_LINE_INTERRUPT;
    exint.line_select   = RX_WAKE_LINE;
    exint.line_polarity = EXINT_TRIGGER_FALLING_EDGE;
    exint.line_enable   = TRUE;
    exint_init(&exint);
    EXINT->inten &= ~RX_WAKE_LINE;
    nvic_irq_enable(EXINT15_10_IRQn, 2, 0);
}

void board_init(void) {
    /* The ROM bootloader may have left interrupts configured */
    __disable_irq();
    SysTick->CTRL = 0;
    for (uint32_t i = 0; i < 8; i++) {
        NVIC->ICER[i] = 0xFFFFFFFFu;
        NVIC->ICPR[i] = 0xFFFFFFFFu;
    }
    SCB->VTOR = (uint32_t)g_pfnVectors;

    clock_init();
    nvic_priority_group_config(NVIC_PRIORITY_GROUP_4);
    SysTick_Config(system_core_clock / 1000);
    __enable_irq();

    rtc_init();
    rx_wake_init();
}

uint32_t board_millis(void) {
    return ticks;
}

void board_idle(void) {
    __WFI();
}

void board_set_fast(bool fast) {
    static bool is_fast;

    if (fast == is_fast) {
        return;
    }
    is_fast = fast;

    lcd_wait_idle();
    __disable_irq();
    crm_auto_step_mode_enable(TRUE);
    if (fast) {
        crm_apb2_div_set(CRM_APB2_DIV_2);
        crm_apb1_div_set(CRM_APB1_DIV_4);
        crm_ahb_div_set(CRM_AHB_DIV_1);
    } else {
        crm_ahb_div_set(CRM_AHB_DIV_2);
        crm_apb2_div_set(CRM_APB2_DIV_1);
        crm_apb1_div_set(CRM_APB1_DIV_2);
    }
    crm_auto_step_mode_enable(FALSE);
    system_core_clock_update();
    SysTick_Config(system_core_clock / 1000);
    __enable_irq();
}

void board_delay_ms(uint32_t ms) {
    uint32_t start = ticks;
    while (ticks - start < ms) {
    }
}

void board_reset(void) {
    NVIC_SystemReset();
}
