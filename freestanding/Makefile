# SPDX-License-Identifier: GPL-2.0-only
# Copyright (C) 2026 matthewdavidrichardanderson

PREFIX := $(DEVKITPPC)/bin/powerpc-eabi-
CC := $(PREFIX)gcc
OBJCOPY := $(PREFIX)objcopy
TARGET := mfb-freestanding
MACH := -mcpu=750 -mno-eabi -mhard-float -mno-sdata -ffreestanding
CFLAGS := $(MACH) -O2 -Wall -Wextra -Werror -ffunction-sections -Iinclude
ASFLAGS := $(MACH) -D_LANGUAGE_ASSEMBLY
LDFLAGS := $(MACH) -nostartfiles -nostdlib -Wl,-T,link.ld,-Map,$(TARGET).map,--gc-sections
OBJS := start.o cache.o memory.o ipc.o es.o di.o patch.o launch.o

.PHONY: all clean
all: $(TARGET).bin

%.o: source/%.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: source/%.S
	$(CC) $(ASFLAGS) -c $< -o $@

$(TARGET).elf: $(OBJS)
	$(CC) $(LDFLAGS) $^ -lgcc -o $@

$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@

clean:
	rm -f $(OBJS) $(TARGET).elf $(TARGET).bin $(TARGET).map
