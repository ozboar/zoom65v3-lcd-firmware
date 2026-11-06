#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#define HEXT_VALUE ((uint32_t)8000000)
#define HEXT_STARTUP_TIMEOUT ((uint16_t)0x3000)
#define HICK_VALUE ((uint32_t)8000000)
#define LEXT_VALUE ((uint16_t)32768)
#define LICK_VALUE ((uint16_t)40000)

#define CRM_MODULE_ENABLED
#define FLASH_MODULE_ENABLED
#define GPIO_MODULE_ENABLED
#define SPI_MODULE_ENABLED
#define DMA_MODULE_ENABLED
#define USART_MODULE_ENABLED
#define MISC_MODULE_ENABLED
#define TMR_MODULE_ENABLED
#define PWC_MODULE_ENABLED
#define EXINT_MODULE_ENABLED
#define RTC_MODULE_ENABLED
#define BPR_MODULE_ENABLED

#include "at32f403a_407_crm.h"
#include "at32f403a_407_dma.h"
#include "at32f403a_407_flash.h"
#include "at32f403a_407_gpio.h"
#include "at32f403a_407_spi.h"
#include "at32f403a_407_usart.h"
#include "at32f403a_407_misc.h"
#include "at32f403a_407_tmr.h"
#include "at32f403a_407_pwc.h"
#include "at32f403a_407_exint.h"
#include "at32f403a_407_rtc.h"
#include "at32f403a_407_bpr.h"

#ifdef __cplusplus
}
#endif
