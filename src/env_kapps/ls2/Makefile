# Makefile do ls.elf (app do Rigel).
# Uso:   make          -> gera ls.elf
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
           -I $(LIBRIGEL)/include  -I$(RIGELDIR)/include -D_PRINTF_FLOAT_NOT_FOUND \
		   -include lixo/fix_winnt.h
		   
# -Wl,-q = guarda as relocacoes (o loader do kernel precisa delas)
# NAO use -s / strip no .elf
LDFLAGS := -m68000 -nostartfiles -T $(LIBRIGEL)/lib/app.ld -Wl,-q -Wl,--gc-sections \
           -L $(LIBRIGEL)/lib $(if $(NEWLIB_LIB),-L $(NEWLIB_LIB))
LDLIBS  := -Wl,--start-group -lrigel -lc -lgcc -Wl,--end-group

all: ls.elf

ls.o: ls.c
	$(CC) $(CFLAGS) -c $< -o $@

ls.elf: ls.o $(LIBRIGEL)/lib/crt0_app.o $(LIBRIGEL)/lib/librigel.a
	$(CC) $(LDFLAGS) $(LIBRIGEL)/lib/crt0_app.o ls.o $(LDLIBS) -o $@

clean:
	rm -f ls.o ls.elf

.PHONY: all clean
