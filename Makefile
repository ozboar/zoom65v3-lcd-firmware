SDK := lib/at32/libraries
DEVICE := $(SDK)/cmsis/cm4/device_support
DRIVERS := $(SDK)/drivers
BUILD := build
STAGE1 := build/stage1

SRCS := $(wildcard src/*.c) \
	$(DEVICE)/system_at32f403a_407.c \
	$(addprefix $(DRIVERS)/src/at32f403a_407_,$(addsuffix .c,crm dma flash gpio spi usart misc tmr pwc exint rtc bpr))
ASMS := $(DEVICE)/startup/gcc/startup_at32f403a_407.s

CFLAGS := -mcpu=cortex-m4 -mthumb -mfloat-abi=soft -Os -g -std=gnu11 -Wall -Wextra \
	-ffunction-sections -fdata-sections \
	-DAT32F403ACGU7 -DUSE_STDPERIPH_DRIVER \
	-Isrc -Ilib/animatedgif -I$(SDK)/cmsis/cm4/core_support -I$(DEVICE) -I$(DRIVERS)/inc
LDFLAGS := -mcpu=cortex-m4 -mthumb -mfloat-abi=soft -specs=nano.specs -specs=nosys.specs \
	-Wl,--gc-sections -Wl,--no-warn-rwx-segments -Tld/sram.ld

OBJS := $(addprefix $(BUILD)/,$(SRCS:.c=.o) $(ASMS:.s=.o))

all: $(BUILD)/lcd.bin $(STAGE1)/stage1.bin

# The release tag when building one, otherwise the git revision
LCD_VERSION := $(shell git describe --tags --exact-match --match 'v*' 2>/dev/null || git rev-parse --short=7 HEAD 2>/dev/null)
# Rebuilds what shows it when it changes
$(shell mkdir -p $(BUILD); [ "$$(cat $(BUILD)/version 2>/dev/null)" = "$(LCD_VERSION)" ] || echo "$(LCD_VERSION)" > $(BUILD)/version)
$(BUILD)/src/ui.o: $(BUILD)/version
$(BUILD)/src/ui.o: CFLAGS += -DLCD_VERSION=\"$(LCD_VERSION)\"

# The GIF decoder is the only part of the firmware short of time, the rest is built for size
$(BUILD)/src/gif.o: CFLAGS += -O2

$(BUILD)/lcd.bin: $(BUILD)/lcd.elf
	arm-none-eabi-objcopy -O binary $< $@
	arm-none-eabi-size $<

$(BUILD)/lcd.elf: $(OBJS) ld/sram.ld
	arm-none-eabi-gcc $(LDFLAGS) $(OBJS) -o $@

$(BUILD)/%.o: %.c
	@mkdir -p $(dir $@)
	arm-none-eabi-gcc $(CFLAGS) -c $< -o $@

$(BUILD)/%.o: %.s
	@mkdir -p $(dir $@)
	arm-none-eabi-gcc $(CFLAGS) -c $< -o $@

# Simulator: the firmware's UI built for this PC, for screenshots. Dumps default to the
# zoom65v3-re repo, whose external flash dump has uploads removed
DUMPS ?= ../zoom65v3-re/dumps
INTERNAL_FLASH ?= $(DUMPS)/screen-mcu.bin
EXTERNAL_FLASH ?= $(DUMPS)/screen-extflash.bin
SIM_SRCS := tools/sim/sim.c $(addprefix src/,ui.c lcd_target.c draw.c gif.c media.c extflash.c assets.c)

$(BUILD)/sim/sim: $(SIM_SRCS) $(wildcard src/*.h) tools/sim/at32f403a_407.h $(BUILD)/version
	@mkdir -p $(dir $@)
	gcc -O1 -std=gnu11 -w -DLCD_VERSION=\"$(LCD_VERSION)\" -Itools/sim -Isrc -Ilib/animatedgif $(SIM_SRCS) -o $@

screenshots: $(BUILD)/sim/sim
	python3 tools/screenshots.py $< $(INTERNAL_FLASH) $(EXTERNAL_FLASH) $(BUILD)/screenshots

clean:
	rm -rf $(BUILD)

.PHONY: all clean screenshots

$(STAGE1)/stage1.bin: stage1/stage1.c ld/stage1.ld
	@mkdir -p $(STAGE1)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -Os -ffreestanding -nostdlib -Wall -Wextra \
		-ffunction-sections -Wl,--gc-sections -Wl,--no-warn-rwx-segments -Tld/stage1.ld stage1/stage1.c -o $(STAGE1)/stage1.elf
	arm-none-eabi-objcopy -O binary $(STAGE1)/stage1.elf $@
	arm-none-eabi-size $(STAGE1)/stage1.elf

