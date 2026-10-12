CROSS   ?= m68k-elf-
CC      := $(CROSS)gcc
OBJCOPY := $(CROSS)objcopy
OBJDUMP := $(CROSS)objdump
SIZE    := $(CROSS)size

TARGET  := app
LDSCRIPT:= app.ld
BUILD   := build

SRCS_C  := $(wildcard src/*.c)
SRCS_S  := $(wildcard src/*.S)
# crt0 sempre primeiro no link (ajuste o caminho)
CRT0    := $(RIGELDIR)/lib/rigel/crt0_app.o
SRCS_S  := $(filter-out $(SRCS_S))

OBJS    := $(BUILD)/crt0.o \
           $(patsubst src/%.c,$(BUILD)/%.o,$(SRCS_C)) \
           $(patsubst src/%.S,$(BUILD)/%.o,$(SRCS_S))

CFLAGS  := -m68000 -Os -g -Wall -ffreestanding -ffunction-sections -fdata-sections -I$(RIGELDIR)/include -I$(RIGELDIR)/include/rigel 
ASFLAGS := -m68000
LDFLAGS := -T $(LDSCRIPT) -nostartfiles -Wl,--gc-sections -Wl,-Map=$(BUILD)/$(TARGET).map -L$(RIGELDIR)/lib -L$(RIGELDIR)/lib/rigel 
LIBS    := -lrigel -lc -lgcc


all: $(BUILD)/$(TARGET).elf $(BUILD)/$(TARGET).bin
	$(SIZE) $(BUILD)/$(TARGET).elf

$(BUILD)/$(TARGET).elf: $(OBJS) $(LDSCRIPT)
	$(CC) $(CFLAGS) $(LDFLAGS) $(CRT0) $(OBJS) $(LIBS) -o $@

$(BUILD)/crt0.o: $(CRT0) | $(BUILD)
	$(CC) $(ASFLAGS) -c $< -o $@

$(BUILD)/%.o: src/%.c | $(BUILD)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

$(BUILD)/%.o: src/%.S | $(BUILD)
	$(CC) $(ASFLAGS) -c $< -o $@

$(BUILD)/$(TARGET).bin: $(BUILD)/$(TARGET).elf
	$(OBJCOPY) -O binary $< $@

$(BUILD):
	mkdir -p $@

disasm: $(BUILD)/$(TARGET).elf
	$(OBJDUMP) -d -S $< > $(BUILD)/$(TARGET).lst

clean:
	rm -rf $(BUILD)

-include $(OBJS:.o=.d)
.PHONY: all clean disasm
