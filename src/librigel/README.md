# librigel -- biblioteca de sistema das aplicacoes do Rigel

Faz a ponte entre a newlib (e os programas) e o kernel, via `TRAP #0`.

## Construir a biblioteca
    make SYSCALL_H=$RIGELDIR/kernel/include/sys/syscall.h

O script `tools/gen_librigel.py` le o `syscall.h` do kernel e gera `libsrc/syscalls_gen.S`
e `include/rigel/*.h`. Se dois SYS_* tiverem o mesmo numero ele para com erro.
Saida em `lib/`: `librigel.a`, `crt0_app.o`, `app.ld`.

## Construir um programa
    m68k-elf-gcc -m68000 -Os -ffreestanding -Ilibrigel/include -c hello.c -o hello.o
    m68k-elf-gcc -m68000 -nostartfiles -T librigel/lib/app.ld -Wl,-q \
        librigel/lib/crt0_app.o hello.o -Llibrigel/lib -L<dir da newlib> \
        -Wl,--start-group -lrigel -lc -lgcc -Wl,--end-group -o hello.elf
    m68k-elf-readelf -r hello.elf        # so deve aparecer R_68K_32 (e PC8/16/32)

`-lrigel` vem ANTES de `-lc` (dentro do grupo), porque a newlib chama write/read/sbrk...
que estao nesta biblioteca.

## Nomes
- glue da newlib, sem prefixo: `_exit open close read write lseek getpid unlink rename mkdir`
  (+ `isatty fstat kill sbrk` em `libsrc/glue.c`)
- servicos do Rigel, com prefixo `rigel_`: `rigel_gotoxy()`, `rigel_spawn()`...
  Para mudar o prefixo: variavel PREFIX em tools/gen_librigel.py.
- A familia antiga SYS_FOPEN/FCLOSE/FREAD/FWRITE/FSYNC/FLSEEK/FIOCTL nao gera wrapper
  (duplicada por SYS_OPEN/CLOSE/READ/WRITE/SINC/LSEEK/IOCTL).
