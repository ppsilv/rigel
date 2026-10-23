# Makefile dos comandos ls, cp, chmod, mv, head e tail (apps do Rigel).
# Uso:   make            -> gera lsr.elf cp.elf mv.elf head.elf tail.elf
#        make head.elf   -> so' um deles
#        make clean
#
# Ajuste so' RIGELDIR se o projeto estiver em outro lugar.
# Se o linker nao achar a newlib (erro "cannot find -lc"), ponha em NEWLIB_LIB a pasta do libc.a
# do m68000, ex.:  make NEWLIB_LIB=/opt/newlib/m68k-elf/lib/m68000

LIBRIGEL  := $(RIGELDIR)/src/librigel
NEWLIB_LIB ?= $(RIGELDIR)/lib

CROSS := m68k-elf-
CC    := $(CROSS)gcc
# librigel/include vem ANTES (headers gerados, os atuais)
CFLAGS  := -m68000 -Wall -Os -ffreestanding -fomit-frame-pointer \
           -ffunction-sections -fdata-sections \
           -I $(LIBRIGEL)/include  -I$(RIGELDIR)/include -D_PRINTF_FLOAT_NOT_FOUND \
		   -I $(RIGELDIR)/src/kernel/include \
		   -include lixo/fix_winnt.h

# -Wl,-q = guarda as relocacoes (o loader do kernel precisa delas). NAO use -s / strip.
LDFLAGS := -m68000 -nostartfiles -T $(LIBRIGEL)/lib/app.ld -Wl,-q -Wl,--gc-sections \
           -L $(LIBRIGEL)/lib $(if $(NEWLIB_LIB),-L $(NEWLIB_LIB))
LDLIBS  := -Wl,--start-group -lrigel -lc -lgcc -Wl,--end-group

APPS := lsr cp chmod mv head tail ps kill

all: $(addsuffix .elf,$(APPS))

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.elf: %.o $(LIBRIGEL)/lib/crt0_app.o $(LIBRIGEL)/lib/librigel.a
	$(CC) $(LDFLAGS) $(LIBRIGEL)/lib/crt0_app.o $< $(LDLIBS) -o $@

clean:
	rm -f $(addsuffix .o,$(APPS)) $(addsuffix .elf,$(APPS))

.PHONY: all clean
