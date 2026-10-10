# Rigel

Sistema operacional homebrew para 68k, baseado na newlib (nano).

## Estrutura

```
boot/crt0.S          vetores + startup (SP, copia .data, zera .bss, chama main)
kernel/duart.[ch]    console serial por polling
kernel/syscalls.c    _write/_read/_sbrk/_exit... (ponte newlib -> hardware)
kernel/main.c        programa de teste (printf, fgets, malloc)
rigel.ld             mapa de memoria
Makefile
```

## Antes de compilar: ajuste 3 coisas

1. `rigel.ld`: `ROM_LENGTH`, `RAM_ORIGIN`, `RAM_LENGTH` (os valores atuais são palpite).
2. `kernel/duart.h`: `DUART_BASE` e os offsets/bits, conferindo com o `duart.c` que já funciona no Orion.
3. `kernel/duart.c`: se o orioncore/Orion já programa baud e 8N1, `duart_init` pode ficar como está; senão, programe MR/CSR/CR.

## Compilar

```
make            # gera rigel.elf, rigel.bin e mostra o tamanho
make clean
```

Requer `m68k-elf-gcc` com newlib (o seu toolchain atual serve). Se o seu
toolchain não tem `nano.specs`, remova `--specs=nano.specs` do Makefile.

## Gravar

`rigel.bin` começa com a tabela de vetores (offset 0). Para a EEPROM de
16 bits em duas metades, use o mesmo `split68k.py` do Orion.

## Próximos passos sugeridos

1. Fazer o "olá" sair na serial (este esqueleto).
2. Trocar polling por interrupção na DUART (buffer circular).
3. VFS + FatFS ligados em `_open/_read/_write/_lseek/_close`.
4. Syscall única por TRAP (número em D0) e scheduler.
