CROSS := m68k-elf-
CC    := $(CROSS)gcc
OBJCOPY = m68k-elf-objcopy

LIBRIGEL  := $(RIGELDIR)/src/librigel
NEWLIB_LIB ?= $(RIGELDIR)/lib

#CFLAGS = -Wall -Os -Wa,-adhln=tinyBasic.lst -I../../include -ffreestanding -nostdlib -D __68000__ -D ALIGN_MEMORY
CFLAGS  := -m68000 -Wall -Os -ffreestanding -fomit-frame-pointer -D __68000__ -D ALIGN_MEMORY -Wa,-adhln=tinyBasic.lst \
           -ffunction-sections -fdata-sections \
           -I $(LIBRIGEL)/include -I$(RIGELDIR)/include

# -Wl,-q = guarda as relocacoes (o loader do kernel precisa delas)
# NAO use -s / strip no .elf
LDFLAGS := -m68000 -nostartfiles -T $(LIBRIGEL)/lib/app.ld -Wl,-q -Wl,--gc-sections \
           -L $(LIBRIGEL)/lib $(if $(NEWLIB_LIB),-L $(NEWLIB_LIB))
LDLIBS  := -Wl,--start-group -lrigel -lc -lgcc -Wl,--end-group

SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)
ELFS = $(SRCS:.c=.elf)
BINS = $(SRCS:.c=.bin)

.PHONY: all clean

#all: $(BINS)
all: tBasic.bin
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Regra específica para cada arquivo para evitar problemas com $<
tBasic.elf: tinyBasic.o TinyBasicPlus.o ../../libc/obj/crt0.o ../../libc/obj/startup.o
	$(CC) $(LDFLAGS) $(OBJS) -o $@ $(LDLIBS)
	@echo "Linked: $@"

test.elf: test.o ../../obj/crt0.o
	$(CC) $(LDFLAGS) ../../obj/crt0.o test.o -lmc68000 -o $@
	@echo "Linked: $@"

%.bin: %.elf
	$(OBJCOPY) -O binary $< $@
	@echo "Binary: $@"

clean:
	rm -f $(OBJS) $(ELFS) $(BINS) tBasic.bin tBasic.elf
	@echo "Clean complete"
