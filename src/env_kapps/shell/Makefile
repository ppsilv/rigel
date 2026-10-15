# Makefile do Rigel
CROSS   ?= m68k-elf-
CC      := $(CROSS)gcc
OBJCOPY := $(CROSS)objcopy
SIZE    := $(CROSS)size


CPU     ?= -m68000
CFLAGS  := $(CPU) -Wall -Os -ffreestanding -fomit-frame-pointer -ffunction-sections -fdata-sections -include lixo/fix_winnt.h\
-I $(RIGELDIR)/include \
-I $(RIGELDIR)/src/kernel/include \
-I $(RIGELDIR)/src/librigel/include \
-I $(RIGELDIR)/src/kernel/include

#LINKER_FILE := $(RIGELDIR)/src/librigel/linker
LINKER_FILE := $(RIGELDIR)/src/librigel/lib/app.ld

CRT0_FILE   := $(RIGELDIR)/src/librigel/lib/crt0_app.o

LDFLAGS := $(CPU) -Wl,-q -T $(LINKER_FILE) -nostartfiles -Wl,--gc-sections -Wl,-Map=rigel.map  -Wl,--undefined=do_load
LDLIBS  := -L$(RIGELDIR)/lib -L$(RIGELDIR)/lib/rigel -Wl,--start-group -lrigel -lc -lgcc -Wl,--end-group

SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)
ELFS = $(SRCS:.c=.elf)
BINS = $(SRCS:.c=.bin)

APP = shell

.PHONY: all clean size

all: $(APP).bin size

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Regra específica para cada arquivo para evitar problemas com $<
$(APP).elf: $(OBJS) 
	$(CC) $(LDFLAGS) $(CRT0_FILE) $(OBJS) -o $@ $(LDLIBS)
	m68k-elf-size -A $(APP).elf
	@echo "Linked: $@"

%.bin: $(APP).elf
	$(OBJCOPY) -O binary $< $@
	@echo "Binary: $@"

clean:
	rm -f $(OBJS).o $(ELFS) $(BINS).bin $(APP).elf
	@echo "Clean complete"
