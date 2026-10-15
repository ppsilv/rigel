# Makefile do wfile.elf e rmfile.elf (apps do Rigel).
# Uso:   make          -> gera wfile.elf e rmfile.elf
#        make clean
#
# Ajuste so' RIGELDIR se o projeto estiver em outro lugar.
# Se o seu linker nao achar a newlib (erro "cannot find -lc"), ponha em NEWLIB_LIB
# a pasta onde esta o libc.a do m68000, ex.:  make NEWLIB_LIB=/opt/newlib/m68k-elf/lib/m68000

LIBRIGEL  := $(RIGELDIR)/src/librigel
NEWLIB_LIB ?= $(RIGELDIR)/lib

CROSS := m68k-elf-
CC    := $(CROSS)gcc

# librigel/include vem ANTES (headers gerados, os atuais)
CFLAGS  := -m68000 -Wall -Os -ffreestanding -fomit-frame-pointer \
           -ffunction-sections -fdata-sections \
           -I $(LIBRIGEL)/include -I$(RIGELDIR)/include

# -Wl,-q = guarda as relocacoes (o loader do kernel precisa delas)
# NAO use -s / strip no .elf
LDFLAGS := -m68000 -nostartfiles -T $(LIBRIGEL)/lib/app.ld -Wl,-q -Wl,--gc-sections \
           -L $(LIBRIGEL)/lib $(if $(NEWLIB_LIB),-L $(NEWLIB_LIB))
LDLIBS  := -Wl,--start-group -lrigel -lc -lgcc -Wl,--end-group

all: touch.elf rm.elf

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.elf: %.o $(LIBRIGEL)/lib/crt0_app.o $(LIBRIGEL)/lib/librigel.a
	$(CC) $(LDFLAGS) $(LIBRIGEL)/lib/crt0_app.o $< $(LDLIBS) -o $@

clean:
	rm -f wfile.o wfile.elf rmfile.o rmfile.elf

.PHONY: all clean
