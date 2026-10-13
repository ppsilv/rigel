# librigel -- biblioteca de sistema das aplicacoes do Rigel

Faz a ponte entre a newlib (e os programas) e o kernel, via `TRAP #0`.

## Construir a biblioteca
    make SYSCALL_H=/caminho/do/kernel/include/sys/syscall.h

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


## Console (teclado + video): modos do /dev/vcon

stdin/stdout/stderr (fd 0, 1, 2) ja' vem ligados ao /dev/vcon pelo kernel.
Por padrao a leitura e' em linha (espera Enter, eco, backspace edita).
Para ler tecla por tecla use `rigel_ioctl` no fd 0 (constantes em `<rigel/tty.h>`,
geradas do vfs.h do kernel -- passe `VFS_H=...` ao `make`):

    #include <rigel/rigel.h>
    rigel_ioctl(0, TIOC_SETMODE, (void *)(TTY_RAW | TTY_NOECHO));   /* liga */
    ...
    rigel_ioctl(0, TIOC_SETMODE, (void *)0);                          /* volta ao normal */

O modo fica no descritor e a tabela de fds e' global: SEMPRE restaure o modo antes de sair.
