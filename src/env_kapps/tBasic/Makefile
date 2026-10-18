# Makefile do Rigel - tBasic
# Para mudar o nome do executavel, altere somente APP.
APP     := tbasic

CROSS   ?= m68k-elf-
CC      := $(CROSS)gcc
OBJCOPY := $(CROSS)objcopy
SIZE    := $(CROSS)size

CPU     ?= -m68000
CFLAGS  := $(CPU) -Wall -Os -ffreestanding -fomit-frame-pointer -D__68000__ \
           -ffunction-sections -fdata-sections -include lixo/fix_winnt.h \
           -I $(RIGELDIR)/include \
           -I $(RIGELDIR)/src/kernel/include \
           -I $(RIGELDIR)/src/librigel/include

LINKER_FILE := $(RIGELDIR)/src/librigel/lib/app.ld
CRT0_FILE   := $(RIGELDIR)/src/librigel/lib/crt0_app.o

LDFLAGS := $(CPU) -Wl,-q -T $(LINKER_FILE) -nostartfiles -Wl,--gc-sections -Wl,-Map=rigel.map -Wl,--undefined=do_load
LDLIBS  := -L$(RIGELDIR)/lib -L$(RIGELDIR)/lib/rigel -Wl,--start-group -lrigel -lc -lgcc -Wl,--end-group

SRCS := $(wildcard *.c)
OBJS := $(SRCS:.c=.o)

.PHONY: all clean size

all: $(APP).elf $(APP).bin size

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(APP).elf: $(OBJS)
	$(CC) $(LDFLAGS) $(CRT0_FILE) $(OBJS) -o $@ $(LDLIBS)
	@echo "Linked: $@"

$(APP).bin: $(APP).elf
	$(OBJCOPY) -O binary $< $@
	@echo "Binary: $@"

size: $(APP).elf
	$(SIZE) -A $(APP).elf
	@ls -l $(APP).elf $(APP).bin

clean:
	rm -f $(OBJS) $(APP).elf $(APP).bin rigel.map
	@echo "Clean complete"