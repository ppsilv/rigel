#!/bin/sh
# Rode DENTRO de src/liblwip:   sh check_mem_cfg.sh
# Mostra qual lwipopts.h o mem.c usa e com quais valores de memoria ele e' compilado.
[ -f src/core/mem.c ] || { echo "rode dentro de src/liblwip"; exit 1; }

CMD=$(make -n -B src/core/mem.o 2>/dev/null | grep -- '-c src/core/mem.c' | head -1)
[ -n "$CMD" ] || { echo "nao achei a linha de compilacao do mem.c"; exit 1; }
BASE=$(echo "$CMD" | sed 's# -c src/core/mem.c.*##')

echo "=== lwipopts.h usado pelo mem.c ==="
$BASE -E -H src/core/mem.c 2>&1 >/dev/null | grep -E "lwipopts\.h|arch/cc\.h|arch/sys_arch\.h" | sort -u

echo "=== valores efetivos ==="
$BASE -E -dM src/core/mem.c 2>/dev/null | grep -E "define (MEM_LIBC_MALLOC|MEMP_MEM_MALLOC|MEM_USE_POOLS|RIGEL_LWIP_OWN_HEAP|LWIP_RAM_HEAP_POINTER|MEM_SIZE|LWIP_ALLOW_MEM_FREE_FROM_OTHER_CONTEXT|LWIP_STATS|MEM_STATS) " | sort

echo "=== simbolos em src/core/mem.o ==="
m68k-elf-nm src/core/mem.o 2>/dev/null | grep -E "mem_malloc|mem_init|rigel_lwip_heap|mem_clib"

echo "=== copias de lwipopts.h no projeto ==="
find "$HOME/project/rigel/rigel" -name lwipopts.h 2>/dev/null
