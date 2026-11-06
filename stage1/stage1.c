/*
 * First stage loader, small enough to load quickly through the ROM bootloader.
 * It receives the firmware into SRAM at a much higher baud rate, then starts it.
 * It has no flash access. Requests are 8 bytes: 0xC5, op, addr (u32 LE), len (u16 LE).
 *   'P' reply: "STUB", program id, 3 zero bytes
 *   'L' addr, len, then len bytes of data. Reply: CRC32 of the data (LE)
 *   'G' addr: starts the program whose vector table is at addr. No reply
 * It sends READY_BYTE once its UART is up, so the keyboard does not have to guess.
 */
#include <stdint.h>

#define REG(addr) (*(volatile uint32_t *)(addr))

#define CRM_CTRL REG(0x40021000)
#define CRM_CFG REG(0x40021004)
#define CRM_CLKINT REG(0x40021008)
#define CRM_APB2EN REG(0x40021018)
#define CRM_BPDC REG(0x40021030)
#define CRM_MISC3 REG(0x40021054)

#define GPIOA_CRH REG(0x40010804)
#define GPIOA_BSRR REG(0x40010810)

#define USART1_STS REG(0x40013800)
#define USART1_DT REG(0x40013804)
#define USART1_BAUD REG(0x40013808)
#define USART1_CTRL1 REG(0x4001380C)
#define USART1_CTRL2 REG(0x40013810)
#define USART1_CTRL3 REG(0x40013814)

#define SCB_VTOR REG(0xE000ED08)
#define SYST_CSR REG(0xE000E010)
#define NVIC_ICER(n) REG(0xE000E180 + (n) * 4)

#define APB2_HZ 120000000u
#define BAUD 3000000u
#define PROGRAM_ID 4
#define READY_BYTE 0x5B

extern uint32_t __stack_top;
void Reset_Handler(void);

__attribute__((section(".vectors"), used)) static const void *const vectors[] = {
    &__stack_top,
    Reset_Handler,
};

/* Matches the stock clock setup: 8 MHz HEXT / 2 * 60 = 240 MHz, APB at 120 MHz */
static void clock_init(void) {
    CRM_CTRL |= 1u;
    while (!(CRM_CTRL & (1u << 1))) {
    }
    CRM_CFG &= ~3u;
    while (CRM_CFG & 0xCu) {
    }
    CRM_CFG = 0;
    CRM_CTRL &= 0xFEF2FFFFu;
    CRM_BPDC = 0;
    CRM_CLKINT = 0x9F0000u;

    CRM_CTRL |= 1u << 16;
    while (!(CRM_CTRL & (1u << 17))) {
    }

    CRM_CFG = (1u << 16) | (1u << 17) | (0xBu << 18) | (3u << 29) | (1u << 31) | (4u << 11) | (4u << 8);
    CRM_MISC3 &= ~(3u << 12);

    CRM_CTRL |= 1u << 24;
    while (!(CRM_CTRL & (1u << 25))) {
    }

    CRM_MISC3 |= 3u << 4;
    CRM_CFG = (CRM_CFG & ~3u) | 2u;
    while (((CRM_CFG >> 2) & 3u) != 2u) {
    }
    CRM_MISC3 &= ~(3u << 4);
}

static void uart_init(void) {
    CRM_APB2EN |= (1u << 0) | (1u << 2) | (1u << 14);
    GPIOA_CRH = (GPIOA_CRH & ~0xFF0u) | 0x890u;
    GPIOA_BSRR = 1u << 10;

    USART1_CTRL1 = 0;
    USART1_CTRL2 = 0;
    USART1_CTRL3 = 0;
    USART1_BAUD = APB2_HZ / BAUD;
    USART1_CTRL1 = (1u << 13) | (1u << 3) | (1u << 2);
}

static uint8_t uart_get(void) {
    while (!(USART1_STS & (1u << 5))) {
    }
    return (uint8_t)USART1_DT;
}

static void uart_put(uint8_t b) {
    while (!(USART1_STS & (1u << 7))) {
    }
    USART1_DT = b;
}

static void receive(uint8_t *out, uint32_t len) {
    uint32_t crc = 0xFFFFFFFFu;

    while (len--) {
        uint8_t b = uart_get();
        *out++    = b;
        crc ^= b;
        for (int i = 0; i < 8; i++) {
            crc = (crc & 1) ? (crc >> 1) ^ 0xEDB88320u : crc >> 1;
        }
    }
    crc = ~crc;
    for (int i = 0; i < 4; i++) {
        uart_put((uint8_t)(crc >> (i * 8)));
    }
}

__attribute__((noreturn)) static void start(uint32_t address) {
    const uint32_t *table = (const uint32_t *)address;

    SCB_VTOR = address;
    __asm volatile(
        "msr msp, %0\n"
        "bx %1\n"
        :: "r"(table[0]), "r"(table[1]));
    __builtin_unreachable();
}

__attribute__((used, noreturn)) void stage1_main(void) {
    SYST_CSR = 0;
    for (int i = 0; i < 8; i++) {
        NVIC_ICER(i) = 0xFFFFFFFFu;
    }

    clock_init();
    uart_init();
    uart_put(READY_BYTE);

    for (;;) {
        uint8_t req[8];

        if (uart_get() != 0xC5) continue;
        for (int i = 1; i < 8; i++) {
            req[i] = uart_get();
        }

        uint32_t address = req[2] | ((uint32_t)req[3] << 8) | ((uint32_t)req[4] << 16) | ((uint32_t)req[5] << 24);
        uint32_t length  = req[6] | ((uint32_t)req[7] << 8);

        if (req[1] == 'P') {
            const uint8_t reply[8] = {'S', 'T', 'U', 'B', PROGRAM_ID, 0, 0, 0};
            for (int i = 0; i < 8; i++) {
                uart_put(reply[i]);
            }
        } else if (req[1] == 'L') {
            receive((uint8_t *)address, length);
        } else if (req[1] == 'G') {
            start(address);
        }
    }
}

__attribute__((naked, noreturn)) void Reset_Handler(void) {
    __asm volatile(
        "cpsid i\n"
        "ldr r0, =__stack_top\n"
        "msr msp, r0\n"
        "b stage1_main\n"
        ".ltorg\n");
}
