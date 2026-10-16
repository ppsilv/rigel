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
     * Kernel ocupa 0x82000 ateh 0xC2000 (256K). Slots de 256K das
     * tasks comecam em 0xC2000 -- NAO pode invadir dai pra frente.
     */
    KERNEL (rwx) : ORIGIN = 0x00082000, LENGTH = 0x00040000
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

    /*
     * Mapa do resto da regiao do kernel (0xC2000 e' o fim):
     *
     *   [ texto/dados/.bss ]  [ HEAP do kmalloc ............ ]  [ pilha de boot ]
     *   0x82000        __kernel_end                  __heap_end   __estack_init
     *
     * A pilha de boot so' e' usada ate' o scheduler comecar (depois cada task usa a propria).
     * O heap comeca logo depois do .bss, onde quer que ele termine -- assim, quando o kernel
     * crescer, o heap se afasta sozinho e nunca pisa no codigo.
     * No kernel.c:   kmalloc_init(__heap_start, __heap_end - __heap_start);
     */
    __estack_init = 0x000C1FF0;
    __estack_end  = __estack_init - 0x2000;          /* 8 KB de pilha de boot */
    __heap_start  = ALIGN(__kernel_end, 4);
    __heap_end    = __estack_end;

    ASSERT(__kernel_end <= __estack_end, "kernel: .bss invade a pilha")
    ASSERT(__heap_end - __heap_start >= 0x10000, "kernel: heap com menos de 64 KB (kernel grande demais)")

    /DISCARD/ : { *(.comment) *(.note*) *(.eh_frame*) }
}
