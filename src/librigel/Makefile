# librigel: biblioteca de sistema das aplicacoes do Rigel.
#   make        (usa $(RIGELDIR)/src/kernel/include/sys/ksyscall.h e vfs.h)   gera stubs+headers e compila
# Saidas em lib/: librigel.a  crt0_app.o  app.ld
CROSS     ?= m68k-elf-
CC        := $(CROSS)gcc
AR        := $(CROSS)ar
# Headers do kernel de onde saem os numeros das syscalls e as constantes do console.
# Por padrao usa $(RIGELDIR)/src/kernel/include/sys/ (RIGELDIR = raiz do projeto Rigel).
# Para outro lugar:  make SYSCALL_H=... VFS_H=...
RIGELDIR  ?= $(HOME)/project/rigel/rigel
SYSCALL_H ?= $(RIGELDIR)/src/kernel/include/sys/ksyscall.h
VFS_H     ?= $(RIGELDIR)/src/kernel/include/sys/vfs.h
DIRENT_H  ?= $(RIGELDIR)/src/kernel/include/sys/rigel_dirent.h
FSFLAGS_H ?= $(RIGELDIR)/src/kernel/include/sys/rigel_fsflags.h
FAMILY    ?= new      # new = SYS_OPEN/READ/WRITE...  old = SYS_FOPEN/FREAD/FWRITE... (a que o trap0.c trata)

CFLAGS  := -m68000 -Os -Wall -ffreestanding -ffunction-sections -fdata-sections -Iinclude -I$(RIGELDIR)/include -include lixo/fix_winnt.h
ASFLAGS := -m68000

all: include/rigel/fsflags.h include/rigel/dirent.h lib/librigel.a lib/crt0_app.o copy

# dirent.h vem do kernel (arquivo mestre) para kernel e apps terem o mesmo layout
include/rigel/dirent.h: FORCE
	cp $(DIRENT_H) $@

# sempre regera: os numeros vem do syscall.h do kernel e nao podem ficar velhos
libsrc/syscalls_gen.S: FORCE
	python3 tools/gen_librigel.py $(SYSCALL_H) $(FAMILY) $(VFS_H)
# fsflags.h vem do kernel (arquivo mestre)
include/rigel/fsflags.h: FORCE
	cp $(FSFLAGS_H) $@

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
	cp -Rf ./include/rigel $(RIGELDIR)/include/.

build:
	mkdir -p build

clean:
	rm -rf build lib/librigel.a lib/crt0_app.o
.PHONY: all clean FORCE
