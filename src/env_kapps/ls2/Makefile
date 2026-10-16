CC = m68k-elf-gcc
LD = m68k-elf-ld
OBJCOPY = m68k-elf-objcopy

ORIONCOR_DIR=../../..
SYS_ROOT=../../../../..
APP = ls2


CFLAGS  := -m68000 -D __68000__  -Wall -Os -ffreestanding -fomit-frame-pointer \
           -ffunction-sections -fdata-sections -Wl,--strip-debug \
           -I $(LIBRIGEL)/include  -I$(RIGELDIR)/include -D_PRINTF_FLOAT_NOT_FOUND \
		   -include lixo/fix_winnt.h

LDFLAGS := -m68000 -nostartfiles -T $(LIBRIGEL)/lib/app.ld -Wl,-q -Wl,--strip-debug -Wl,--gc-sections \
           -L $(LIBRIGEL)/lib $(if $(NEWLIB_LIB),-L $(NEWLIB_LIB))
LDLIBS  := -Wl,--start-group -lrigel -lc -lgcc -Wl,--end-group



SRCS = $(APP).c \
../../apps_libs/fatfs/ff.c \
../../apps_libs/fatfs/ffsystem.c \
../../apps_libs/fatfs/ffunicode.c \
../../apps_libs/ide/ata.c \
../../apps_libs/picoVga/drv_picoVga.c \
../../apps_libs/ide/diskio.c

OBJS = $(SRCS:.c=.o)
ELFS = $(SRCS:.c=.elf)
BINS = $(SRCS:.c=.bin)

.PHONY: all clean

all: $(APP).bin
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Regra específica para cada arquivo para evitar problemas com $<
$(APP).elf: $(OBJS)
	$(LD) $(LDFLAGS) $(OBJS) -o $@ $(LDLIBS)
	@echo "Linked: $@"

%.bin: %.elf
	$(OBJCOPY) -O binary $< $@
	@echo "Binary: $@"

clean:
	rm -f $(OBJS) $(ELFS) $(BINS) *.lst
	@echo "Clean complete"
