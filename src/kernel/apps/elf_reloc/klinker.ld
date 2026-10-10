/* kernel.ld -- imagem de endereço fixo em 0x82000 (carregada pelo Rigel) */
ENTRY(_start)

STACK_SIZE = 0x2000;              /* ajuste */

MEMORY
{
    /* 0x80000-0x803FF: stubs de salto; 0x80400-0x81FFF: Rigel (.data/.bss/heap) */
    RAM (rwx) : ORIGIN = 0x82000, LENGTH = 0xFFFF0 - 0x82000
}

SECTIONS
{
    .text : {
        *(.text.start)            /* _start primeiro, se existir essa seção */
        *(.text .text.*)
        *(.gnu.linkonce.t.*)
        . = ALIGN(4);
    } > RAM

    .rodata : {
        *(.rodata .rodata.*)
        . = ALIGN(4);
    } > RAM

    .data : {
        __data_start = .;
        *(.data .data.*)
        . = ALIGN(4);
        __data_end = .;
    } > RAM

    .bss (NOLOAD) : {
        . = ALIGN(4);
        __bss_start = .;
        *(.bss .bss.* COMMON)
        . = ALIGN(4);
        __bss_end = .;
    } > RAM

    _end = .;
    end  = .;

    /* a pilha fica no topo da RAM; o que sobrar entre _end e ela é heap */
    _stack_top  = ORIGIN(RAM) + LENGTH(RAM);
    _heap_limit = _stack_top - STACK_SIZE;

    ASSERT(_end <= _heap_limit, "kernel nao cabe: _end invade a pilha")

    /DISCARD/ : { *(.comment) *(.note*) *(.eh_frame*) }
}