# librigel: biblioteca de sistema das aplicacoes do Rigel.
#   make SYSCALL_H=/caminho/sys/syscall.h VFS_H=/caminho/sys/vfs.h   gera stubs+headers e compila
# Saidas em lib/: librigel.a  crt0_app.o  app.ld
CROSS     ?= m68k-elf-
CC        := $(CROSS)gcc
AR        := $(CROSS)ar
SYSCALL_H ?= ../kernel/include/sys/syscall.h
VFS_H     ?= ../kernel/include/sys/vfs.h      # de onde saem as constantes TIOC_*/TTY_* (rigel/tty.h)
FAMILY    ?= new      # new = SYS_OPEN/READ/WRITE...  old = SYS_FOPEN/FREAD/FWRITE... (a que o trap0.c trata)

CFLAGS  := -m68000 -Os -Wall -ffreestanding -ffunction-sections -fdata-sections -Iinclude
ASFLAGS := -m68000

all: lib/librigel.a lib/crt0_app.o copy

# sempre regera: os numeros vem do syscall.h do kernel e nao podem ficar velhos
libsrc/syscalls_gen.S: FORCE
	python3 tools/gen_librigel.py $(SYSCALL_H) $(FAMILY) $(VFS_H)
FORCE:

build/%.o: libsrc/%.S | build
	$(CC) $(ASFLAGS) -c $< -o $@
build/%.o: libsrc/%.c | build
	$(CC) $(CFLAGS) -c $< -o $@

lib/librigel.a: build/syscalls_gen.o build/glue.o
	$(AR) rcs $@ $^

lib/crt0_app.o: libsrc/crt0_app.S
	$(CC) $(ASFLAGS) -c $< -o $@

copy:
	cp ./lib/librigel.a $(RIGELDIR)/lib/.

build:
	mkdir -p build

clean:
	rm -rf build lib/librigel.a lib/crt0_app.o
.PHONY: all clean FORCE
