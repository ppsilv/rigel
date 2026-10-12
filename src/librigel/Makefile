# librigel: biblioteca de sistema das aplicacoes do Rigel.
#   make SYSCALL_H=/caminho/sys/syscall.h     gera stubs+headers e compila
# Saidas em lib/: librigel.a  crt0_app.o  app.ld
CROSS     ?= m68k-elf-
CC        := $(CROSS)gcc
AR        := $(CROSS)ar
SYSCALL_H ?= $(RIGELDIR)/src/kernel/include/sys/syscall.h
FAMILY    ?= new      # new = SYS_OPEN/READ/WRITE...  old = SYS_FOPEN/FREAD/FWRITE... (a que o trap0.c trata)

CFLAGS  := -m68000 -Os -Wall -ffreestanding -ffunction-sections -fdata-sections -I$(RIGELDIR)/include -I$(RIGELDIR)/include/rigel
ASFLAGS := -m68000

all: $(RIGELDIR)/lib/rigel/librigel.a $(RIGELDIR)/lib/rigel/crt0_app.o

# sempre regera: os numeros vem do syscall.h do kernel e nao podem ficar velhos
#libsrc/syscalls_gen.S: FORCE
#	python3 tools/gen_librigel.py $(SYSCALL_H) $(FAMILY)
#FORCE:

build/%.o: libsrc/%.S | build
	$(CC) $(ASFLAGS) -c $< -o $@
build/%.o: libsrc/%.c | build
	$(CC) $(CFLAGS) -c $< -o $@

$(RIGELDIR)/lib/rigel/librigel.a: build/syscalls_gen.o build/glue.o
	$(AR) rcs $@ $^

$(RIGELDIR)/lib/rigel/crt0_app.o: libsrc/crt0_app.S
	$(CC) $(ASFLAGS) -c $< -o $@

build:
	mkdir -p build

clean:
	rm -rf build lib/librigel.a lib/crt0_app.o
.PHONY: all clean FORCE
