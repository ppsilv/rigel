# librigel -- biblioteca de sistema das aplicacoes do Rigel

Faz a ponte entre a newlib (e os programas) e o kernel, via `TRAP #0`.

## Construir a biblioteca
    make SYSCALL_H=$RIGELDIR/kernel/include/sys/syscall.h

O script `tools/gen_librigel.py` le o `syscall.h` do kernel e gera `libsrc/syscalls_gen.S`
e `include/rigel/*.h`. Se dois SYS_* tiverem o mesmo numero ele para com erro.
Saida em `lib/`: `librigel.a`, `crt0_app.o`, `app.ld`.

## Construir um programa
m68k-elf-gcc -m68000 -Os -ffreestanding -c tstsys.c -I $RIGELDIR/include -I ./include -DUSE_PRINTF
m68k-elf-gcc -m68000 -nostartfiles -Wl,-q -T $RIGELDIR/src/env_kapps/app.ld $RIGELDIR/lib/rigel/crt0_app.o tstsys.o  -L$RIGELDIR/lib -L$RIGELDIR/lib/rigel -Wl,--start-group -lrigel -lc -lgcc -Wl,--end-group -o tstsys.elf




`-lrigel` vem ANTES de `-lc` (dentro do grupo), porque a newlib chama write/read/sbrk...
que estao nesta biblioteca.

## Nomes
- glue da newlib, sem prefixo: `_exit open close read write lseek getpid unlink rename mkdir`
  (+ `isatty fstat kill sbrk` em `libsrc/glue.c`)
- servicos do Rigel, com prefixo `rigel_`: `rigel_gotoxy()`, `rigel_spawn()`...
  Para mudar o prefixo: variavel PREFIX em tools/gen_librigel.py.
- A familia antiga SYS_FOPEN/FCLOSE/FREAD/FWRITE/FSYNC/FLSEEK/FIOCTL nao gera wrapper
  (duplicada por SYS_OPEN/CLOSE/READ/WRITE/SINC/LSEEK/IOCTL).
