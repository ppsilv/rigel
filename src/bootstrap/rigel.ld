/*
 * rigel.ld -- mapa de memoria do Rigel
 *
 * Layout:
 *   ROM: vetores (offset 0) + codigo + constantes + copia inicial do .data
 *   RAM: .data (copiado da ROM no boot), .bss, heap (cresce pra cima)
 *        e pilha (comeca no topo da RAM e cresce pra baixo)
 *
 *   RAM:  [.data][.bss][ ... heap -> ...   <- pilha ]
 *                      _end              _heap_limit   _stack_top
 */

ENTRY(_start)

/* ---------- AJUSTE ESTES VALORES PARA A SUA PLACA ---------- */
ROM_ORIGIN = 0x00000000;
ROM_LENGTH = 64K;          /* AT29C512 = 64K, AT28C256 = 32K */
RAM_ORIGIN = 0x00080404;   /* CONFIRME: onde comeca a sua RAM */
RAM_LENGTH = 512k;
STACK_SIZE = 16K;
/* ----------------------------------------------------------- */

MEMORY
{
    ROM (rx)  : ORIGIN = ROM_ORIGIN, LENGTH = ROM_LENGTH
    RAM (rwx) : ORIGIN = RAM_ORIGIN, LENGTH = RAM_LENGTH
}

SECTIONS
{
    . = 0x00000000;

    .vectors :
    {
        KEEP(*(.vectors))
        
        /* Força a seção de vetores a preencher EXATAMENTE os primeiros 1024 bytes (0x400) */
        /* Isso protege o espaço das 256 exceções possíveis do Motorola 68000 */
        . = ALIGN(0x400); 
    } > ROM

    .text : {
        . = ALIGN(2);
        *(.text*)
        *(.rodata)
        . = ALIGN(2);
    } > ROM

    /* As constantes de texto e tabelas de leitura também ficam na ROM */
    .rodata : {
        . = ALIGN(2);
        *(.rodata*)
        . = ALIGN(2);
    } > ROM

    /* A SEÇÃO .data: Ela roda (VMA) na RAM, mas seus valores iniciais
       ficam salvos fisicamente (LMA) na ROM, logo após o .rodata */
    .data : {
        . = ALIGN(2);
        __data_start = .;   /* Início do bloco na RAM */
        *(.data*)
        . = ALIGN(2);
        __data_end = .;     /* Fim do bloco na RAM */
    } > RAM AT> ROM

    .mram  (NOLOAD) :
    {
        *(?.mram)
    } > RAM

    /* Guarda o endereço físico na ROM de onde o startup deve copiar os dados */
    __data_load_start = LOADADDR(.data);

    /* Variáveis globais não inicializadas ficam direto na RAM */
    .bss (NOLOAD) : {
        . = ALIGN(2);
        __bss_start = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(2);
        __bss_end = .;
    } > RAM


    _end = .;                      /* o heap (_sbrk) comeca aqui */
    end  = .;


    /DISCARD/ : {
        *(.comment)
        *(.note*)
        *(.align*)
    }    
}

/*ASSERT(_end <= _heap_limit, "Rigel: RAM insuficiente para .data + .bss + pilha")*/
