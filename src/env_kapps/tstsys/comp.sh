#!/bin/bash

m68k-elf-gcc -m68000 -Os -ffreestanding -c tstsys.c -I $RIGELDIR/include -I ./include -DUSE_PRINTF
m68k-elf-gcc -m68000 -nostartfiles -Wl,-q -T $RIGELDIR/src/env_kapps/app.ld $RIGELDIR/lib/rigel/crt0_app.o tstsys.o \
-L$RIGELDIR/lib -L$RIGELDIR/lib/rigel -Wl,--start-group -lrigel -lc -lgcc -Wl,--end-group -o tstsys.elf