CROSS   ?= m68k-elf-
CC      := $(CROSS)gcc
OBJCOPY := $(CROSS)objcopy
OBJDUMP := $(CROSS)objdump
SIZE    := $(CROSS)size

TARGET  := kernel
LDSCRIPT:= kernel.ld
BUILD   := build

SRCS_C  := $(wildcard src/*.c)
SRCS_S  := $(wildcard src/*.S)
# crt0 sempre primeiro no link (ajuste o caminho)
CRT0    := src/crt0.S
SRCS_S  := $(filter-out $(CRT0),$(SRCS_S))

OBJS    := $(BUILD)/crt0.o \
           $(patsubst src/%.c,$(BUILD)/%.o,$(SRCS_C)) \
           $(patsubst src/%.S,$(BUILD)/%.o,$(SRCS_S))

CFLAGS  := -m68000 -Os -g -Wall -ffreestanding -ffunction-sections -fdata-sections -Iinclude
ASFLAGS := -m68000
LDFLAGS := -T $(LDSCRIPT) -nostartfiles -Wl,--gc-sections -Wl,-Map=$(BUILD)/$(TARGET).map
LIBS    := -lc -lgcc

MODE ?= kernel                  # make            -> kernel (0x82000)
                                # make MODE=app   -> app (link em 0 + -Wl,-q)
ifeq ($(MODE),kernel)
  DEFS     := -DBUILD_KERNEL
  LDSCRIPT := kernel.ld
  BUILD    := build/kernel
else
  DEFS     := -DBUILD_APP
  LDSCRIPT := app.ld
  EXTRA_LD := -Wl,-q            # mantém as relocações para o elf_reloc
  BUILD    := build/app
endif

CFLAGS  += $(DEFS)
ASFLAGS += $(DEFS)
LDFLAGS += $(EXTRA_LD)

all: $(BUILD)/$(TARGET).elf $(BUILD)/$(TARGET).bin
	$(SIZE) $(BUILD)/$(TARGET).elf

$(BUILD)/$(TARGET).elf: $(OBJS) $(LDSCRIPT)
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) $(LIBS) -o $@

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