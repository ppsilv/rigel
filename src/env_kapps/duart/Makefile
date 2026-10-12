# Makefile do Rigel
CROSS   ?= m68k-elf-
CC      := $(CROSS)gcc
OBJCOPY := $(CROSS)objcopy
SIZE    := $(CROSS)size


CPU     ?= -m68000
CFLAGS  := $(CPU) -Wall -Os -ffreestanding -fomit-frame-pointer -ffunction-sections -fdata-sections -I kernel -I $(RIGELDIR)/include -I ./include
LDFLAGS := $(CPU) -Wl,-q  -T $(RIGELDIR)/src/env_kapps/app.ld -nostartfiles -Wl,--gc-sections -Wl,-Map=rigel.map
# crt0 e' nosso; libc/libgcc entram automaticamente
LDLIBS  := -L$(RIGELDIR)/lib -L$(RIGELDIR)/lib/rigel -Wl,--start-group -lrigel -lc -lgcc -Wl,--end-group


CRT0    := $(RIGELDIR)/lib/rigel/crt0_app.o

SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)
ELFS = $(SRCS:.c=.elf)
BINS = $(SRCS:.c=.bin)

APP = duart

.PHONY: all clean size

all: $(APP).bin size

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Regra específica para cada arquivo para evitar problemas com $<
$(APP).elf: $(OBJS) 
	$(CC) $(LDFLAGS) $(CRT0) $(OBJS) -o $@ $(LDLIBS)
	m68k-elf-size -A $(APP).elf
	@echo "Linked: $@"

%.bin: $(APP).elf
	$(OBJCOPY) -O binary $< $@
	@echo "Binary: $@"

clean:
	rm -f $(OBJS).o $(ELFS) $(BINS).bin $(APP).elf
	@echo "Clean complete"
