#!/bin/bash

m68k-elf-gcc -m68000 -Os -ffreestanding -c tsttrap.c 
m68k-elf-gcc -m68000 -nostartfiles -nostdlib -Wl,-q -T $RIGELDIR/src/env_kapps/app.ld $RIGELDIR/lib/rigel/crt0_app.o tsttrap.o -o tsttrap.elf