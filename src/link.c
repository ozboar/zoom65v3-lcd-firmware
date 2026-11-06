#include <stdbool.h>
#include <string.h>
#include "at32f403a_407.h"
#include "board.h"
#include "link.h"
#include "media.h"
#include "ui.h"

/* Protocol: see docs/protocol.md */
#define BAUD 1500000
#define FRAME_START 0xA5
#define GROUP_CONTROL 0x00
#define GROUP_DATA 0x01
#define GROUP_MEDIA 0x02
#define GROUP_UI 0x03
#define MEDIA_DELETE_IMAGE 0xE0
#define MEDIA_DELETE_GIF 0xE1
#define MEDIA_START 0xF0
#define MEDIA_LENGTH 0xD0
#define MEDIA_END 0xF1
#define MAX_PACKET 30
#define CONTROL_RESET 0xFF
#define CONTROL_NEXT 0x20
#define CONTROL_DOWN 0x21
#define CONTROL_UP 0x22

#define DATA_TIME 0x10
#define DATA_WEATHER 0x20
#define DATA_SYSTEM 0x40
#define DATA_PAGE 0x70
#define DATA_THEME 0xFF

#define REPLY_OK 1
#define REPLY_REJECTED 0

#define PING_START 0xC5
#define PING_LENGTH 8
#define PROGRAM_ID 3
#define READY_BYTE 0x5A
#define READY_DELAY_MS 10

#define MAX_PAYLOAD 20
/* Holds the longest burst of UI frames while the main loop is drawing */
#define RING_SIZE 1024
/* USART1 RX. The CPU stalls for longer than a byte takes during flash operations, so it cannot be left to an interrupt */
#define RX_DMA DMA1_CHANNEL5
/* Longer than the keyboard stalls mid frame, which it does for about 2 ms on every LED update */
#define FRAME_GAP_MS 10

enum {
    UI_TAP = 0x01,
    UI_WPM = 0x02,
    UI_STATUS = 0x03,
    UI_KEYS = 0x04,
    UI_MODE = 0x05,
    UI_POWER = 0x06,
    UI_BACKLIGHT = 0x07,
    UI_LAYOUT = 0x08,
    UI_KEY_TAP = 0x09,
    UI_ACCENT = 0x0A,
    UI_UNITS = 0x0B,
    UI_STATUS_EXTRA = 0x0C,
    UI_RGB = 0x0D,
    UI_RGB_NAME = 0x0E,
    UI_CLOCK_FACE = 0x0F,
    UI_BUILD = 0x10,
    UI_VERSIONS = 0x11,
    UI_MEDIA = 0x12,
    UI_BRIGHTNESS = 0x13,
    UI_WAKE = 0x14,
};

typedef enum { RX_IDLE, RX_GROUP, RX_COMMAND, RX_PAYLOAD, RX_SKIP, RX_SKIP_OK, RX_PING, RX_PACKET_INDEX, RX_PACKET } rx_state_t;

static volatile uint8_t  ring[RING_SIZE];
static uint16_t          ring_tail;
static uint32_t          last_byte;

static rx_state_t state;
static uint8_t    group;
static uint8_t    command;
static uint8_t    payload[MAX_PAYLOAD];
static uint8_t    expected;
static uint8_t    received;
static uint8_t    packet[MAX_PACKET];
static uint8_t    packet_length;

static void send(const void *data, uint32_t len) {
    const uint8_t *bytes = data;
    while (len--) {
        while (!usart_flag_get(USART1, USART_TDBE_FLAG)) {
        }
        USART1->dt = *bytes++;
    }
}

static void rx_dma_init(void) {
    dma_init_type dma;

    crm_periph_clock_enable(CRM_DMA1_PERIPH_CLOCK, TRUE);
    dma_reset(RX_DMA);
    dma_default_para_init(&dma);
    dma.peripheral_base_addr  = (uint32_t)&USART1->dt;
    dma.memory_base_addr      = (uint32_t)ring;
    dma.direction             = DMA_DIR_PERIPHERAL_TO_MEMORY;
    dma.buffer_size           = RING_SIZE;
    dma.peripheral_inc_enable = FALSE;
    dma.memory_inc_enable     = TRUE;
    dma.peripheral_data_width = DMA_PERIPHERAL_DATA_WIDTH_BYTE;
    dma.memory_data_width     = DMA_MEMORY_DATA_WIDTH_BYTE;
    dma.loop_mode_enable      = TRUE;
    dma.priority              = DMA_PRIORITY_VERY_HIGH;
    dma_init(RX_DMA, &dma);
    dma_channel_enable(RX_DMA, TRUE);
}

static uint16_t ring_head(void) {
    return (RING_SIZE - dma_data_number_get(RX_DMA)) % RING_SIZE;
}

void link_init(void) {
    gpio_init_type gpio;

    crm_periph_clock_enable(CRM_GPIOA_PERIPH_CLOCK, TRUE);
    crm_periph_clock_enable(CRM_USART1_PERIPH_CLOCK, TRUE);

    gpio_default_para_init(&gpio);
    gpio.gpio_drive_strength = GPIO_DRIVE_STRENGTH_STRONGER;
    gpio.gpio_out_type       = GPIO_OUTPUT_PUSH_PULL;
    gpio.gpio_mode           = GPIO_MODE_MUX;
    gpio.gpio_pins           = GPIO_PINS_9;
    gpio_init(GPIOA, &gpio);

    gpio.gpio_mode = GPIO_MODE_INPUT;
    gpio.gpio_pull = GPIO_PULL_UP;
    gpio.gpio_pins = GPIO_PINS_10;
    gpio_init(GPIOA, &gpio);

    usart_init(USART1, BAUD, USART_DATA_8BITS, USART_STOP_1_BIT);
    usart_transmitter_enable(USART1, TRUE);
    usart_receiver_enable(USART1, TRUE);
    rx_dma_init();
    usart_dma_receiver_enable(USART1, TRUE);
    usart_enable(USART1, TRUE);

    /*
     * Frames are buffered from here on, so the keyboard can start sending straight away.
     * The wait gives it time to leave the first stage's baud rate, or it would miss this.
     */
    board_delay_ms(READY_DELAY_MS);
    const uint8_t ready = READY_BYTE;
    send(&ready, 1);
}

static int payload_length(uint8_t cmd) {
    switch (cmd) {
        case UI_TAP:
        case UI_WAKE:
            return 0;
        case UI_WPM:
        case UI_KEY_TAP:
            return 2;
        case UI_STATUS:
            return 4;
        case UI_KEYS:
            return 3;
        case UI_RGB:
            return 6;
        case UI_RGB_NAME:
        case UI_BUILD:
        case UI_VERSIONS:
            return MAX_PAYLOAD;
        case UI_MEDIA:
        case UI_CLOCK_FACE:
        case UI_STATUS_EXTRA:
        case UI_ACCENT:
        case UI_UNITS:
        case UI_MODE:
        case UI_POWER:
        case UI_BACKLIGHT:
        case UI_BRIGHTNESS:
        case UI_LAYOUT:
            return 1;
        default:
            return -1;
    }
}

/* Stock commands that zoom-sync sends. It treats a rejection as a failure */
static int data_payload_length(uint8_t cmd) {
    switch (cmd) {
        case DATA_TIME:
            return 6;
        case DATA_WEATHER:
        case DATA_SYSTEM:
            return 4;
        case DATA_PAGE:
            return 1;
        default:
            return -1;
    }
}

static int media_payload_length(uint8_t cmd) {
    switch (cmd) {
        case MEDIA_DELETE_IMAGE:
        case MEDIA_DELETE_GIF:
            return 0;
        case MEDIA_START:
        case MEDIA_END:
            return 1;
        case MEDIA_LENGTH:
            return 4;
        default:
            return -1;
    }
}

static void send_reply(uint8_t value) {
    send(&value, 1);
}

static void dispatch_data(void) {
    if (command == DATA_SYSTEM) {
        ui_on_system(payload[0], payload[1], (uint16_t)((payload[2] << 8) | payload[3]));
    } else if (command == DATA_TIME) {
        ui_on_time(payload[0], payload[1], payload[2], payload[3], payload[4], payload[5]);
    } else if (command == DATA_WEATHER) {
        ui_on_weather(payload[0], payload[1], payload[2], payload[3]);
    }
    send_reply(REPLY_OK);
}

static void dispatch_media(void) {
    bool ok = false;

    if (command == MEDIA_START) {
        ok = media_upload_start(payload[0]);
    } else if (command == MEDIA_LENGTH) {
        ok = media_upload_length(((uint32_t)payload[0] << 24) | ((uint32_t)payload[1] << 16) | ((uint32_t)payload[2] << 8) | payload[3]);
    } else if (command == MEDIA_END) {
        ok = media_upload_end();
    } else {
        ok = media_delete(command);
    }
    send_reply(ok ? REPLY_OK : REPLY_REJECTED);
}

static void dispatch(void) {
    if (group == GROUP_DATA) {
        dispatch_data();
        return;
    }
    if (group == GROUP_MEDIA) {
        dispatch_media();
        return;
    }

    switch (command) {
        case UI_TAP:
            ui_on_tap();
            break;
        case UI_KEY_TAP:
            ui_on_key_tap(payload[0], payload[1]);
            break;
        case UI_ACCENT:
            ui_on_accent(payload[0]);
            break;
        case UI_UNITS:
            ui_on_units(payload[0]);
            break;
        case UI_CLOCK_FACE:
            ui_on_clock_face(payload[0]);
            break;
        case UI_MEDIA:
            ui_on_media(payload[0]);
            break;
        case UI_BUILD:
            ui_on_build((const char *)payload);
            break;
        case UI_VERSIONS:
            ui_on_versions((const char *)payload, (uint16_t)((payload[MAX_PAYLOAD - 2] << 8) | payload[MAX_PAYLOAD - 1]));
            break;
        case UI_STATUS_EXTRA:
            ui_on_status_extra(payload[0]);
            break;
        case UI_RGB:
            ui_on_rgb(payload[0] != 0, payload[1], payload[2], payload[3], payload[4], payload[5]);
            break;
        case UI_RGB_NAME:
            payload[MAX_PAYLOAD - 1] = 0;
            ui_on_rgb_name((const char *)payload);
            break;
        case UI_WPM:
            ui_on_wpm((uint16_t)((payload[0] << 8) | payload[1]));
            break;
        case UI_STATUS:
            ui_on_status(payload[0], payload[1], payload[2] != 0, payload[3]);
            break;
        case UI_KEYS:
            ui_on_keys(payload[0], (uint16_t)((payload[1] << 8) | payload[2]));
            break;
        case UI_MODE:
            ui_on_mode(payload[0]);
            break;
        case UI_POWER:
            ui_on_power(payload[0] != 0);
            break;
        case UI_WAKE: {
            /* Replies first, as waking the panel takes a while */
            const uint8_t ready = READY_BYTE;
            send(&ready, 1);
            ui_on_power(true);
            break;
        }
        case UI_BRIGHTNESS:
            ui_on_brightness(payload[0]);
            break;
        case UI_BACKLIGHT:
            ui_on_backlight(payload[0] != 0);
            break;
        case UI_LAYOUT:
            ui_on_layout(payload[0] != 0);
            break;
        default:
            break;
    }
}

static void on_command(uint8_t value) {
    command = value;

    if (group == GROUP_CONTROL && command == CONTROL_RESET) {
        board_reset();
    }

    /* Screen navigation is the keyboard's job here, but zoom-sync still expects an ok */
    if (group == GROUP_CONTROL && (command == CONTROL_NEXT || command == CONTROL_DOWN || command == CONTROL_UP)) {
        send_reply(REPLY_OK);
        state = RX_IDLE;
        return;
    }

    /* The theme command has an optional argument, so it ends when the line goes quiet */
    if (group == GROUP_DATA && command == DATA_THEME) {
        state = RX_SKIP_OK;
        return;
    }

    int length = group == GROUP_UI ? payload_length(command) : group == GROUP_DATA ? data_payload_length(command) : group == GROUP_MEDIA ? media_payload_length(command) : -1;

    /* Not ours: wait for the line to go quiet, then reject it as stock would */
    if (group != GROUP_UI && length < 0) {
        state = RX_SKIP;
        return;
    }
    if (length < 0) {
        state = RX_IDLE;
        return;
    }

    expected = (uint8_t)length;
    received = 0;
    if (expected == 0) {
        dispatch();
        state = RX_IDLE;
    } else {
        state = RX_PAYLOAD;
    }
}

static void on_byte(uint8_t value) {
    switch (state) {
        case RX_IDLE:
            /* Data packets have no header, so the expected index is what identifies one */
            if (media_upload_receiving() && value == (uint8_t)(media_next_index() >> 8)) {
                packet[0] = value;
                state     = RX_PACKET_INDEX;
            } else if (value == FRAME_START) {
                state = RX_GROUP;
            } else if (value == PING_START) {
                received = 1;
                state    = RX_PING;
            } else if (media_upload_receiving()) {
                media_upload_cancel();
                state = RX_SKIP;
            }
            break;
        case RX_PACKET_INDEX:
            if (value == (uint8_t)media_next_index()) {
                packet[1]     = value;
                received      = 2;
                packet_length = media_packet_length();
                state         = RX_PACKET;
            } else if (packet[0] == FRAME_START) {
                group = value;
                state = RX_COMMAND;
            } else {
                media_upload_cancel();
                state = RX_SKIP;
            }
            break;
        case RX_PACKET:
            packet[received++] = value;
            if (received == packet_length) {
                send_reply(media_on_packet(packet, packet_length) ? REPLY_OK : REPLY_REJECTED);
                state = RX_IDLE;
            }
            break;
        case RX_GROUP:
            group = value;
            state = RX_COMMAND;
            break;
        case RX_COMMAND:
            on_command(value);
            break;
        case RX_PAYLOAD:
            payload[received++] = value;
            if (received == expected) {
                dispatch();
                state = RX_IDLE;
            }
            break;
        case RX_PING:
            if (++received == PING_LENGTH) {
                const uint16_t sleeps   = board_sleep_count();
                const uint8_t  reply[8] = {'S', 'T', 'U', 'B', PROGRAM_ID, board_rtc_source(), (uint8_t)(sleeps >> 8), (uint8_t)sleeps};
                send(reply, sizeof(reply));
                state = RX_IDLE;
            }
            break;
        case RX_SKIP:
        case RX_SKIP_OK:
            break;
    }
}

bool link_rx_idle(void) {
    return ring_tail == ring_head() && gpio_input_data_bit_read(GPIOA, GPIO_PINS_10) == SET;
}

bool link_quiet(uint32_t ms) {
    return state == RX_IDLE && link_rx_idle() && board_millis() - last_byte >= ms;
}

void link_on_wake(void) {
    (void)USART1->sts;
    (void)USART1->dt;
    ring_tail = ring_head();
    state     = RX_IDLE;
    last_byte = board_millis();
}

void link_poll(void) {
    while (ring_tail != ring_head()) {
        on_byte(ring[ring_tail]);
        ring_tail = (ring_tail + 1) % RING_SIZE;
        last_byte = board_millis();
    }

    if (state != RX_IDLE && board_millis() - last_byte >= FRAME_GAP_MS) {
        if (state == RX_SKIP) {
            send_reply(REPLY_REJECTED);
        } else if (state == RX_SKIP_OK) {
            send_reply(REPLY_OK);
        }
        state = RX_IDLE;
    }
}
