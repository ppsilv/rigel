OUTPUT_FORMAT("elf32-m68k")
OUTPUT_ARCH(m68k)

/*
 * ENTRY(_start): diferente do script das aplicacoes, o kernel NAO e'
 * chamado como funcao C por um loader -- o orioncore faz jmp 0x82000
 * direto no primeiro byte do binario flat. Por isso precisa de um
 * stub em assembly (_start, em crt0_kernel.s) garantidamente primeiro
 * no .text, que ajusta o SP e zera o .bss antes de chamar kernel_main.
 */
ENTRY(_start)

MEMORY
{
    /*
     * Kernel ocupa 0x82000 ateh 0xA2000 (128K). Slots de 256K das
     * tasks comecam em 0xA2000 -- NAO pode invadir dai pra frente.
     */
    KERNEL (rwx) : ORIGIN = 0x00082000, LENGTH = 0x00020000
}

SECTIONS
{
    . = ORIGIN(KERNEL);

    .text : {
        crt0.o(.text)
        *(.text*)
        *(.rodata*)
        . = ALIGN(4);
    } > KERNEL

    .data : {
        *(.data*)
        . = ALIGN(4);
    } > KERNEL

    .bss : {
        __bss_start = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        __bss_end = .;
    } > KERNEL

    __kernel_end = .;
    __estack = 0x000A1FF0;

    ASSERT(__kernel_end <= __estack, "kernel: .bss invade a pilha")
    
    /DISCARD/ : { *(.comment) *(.note*) *(.eh_frame*) }
}
