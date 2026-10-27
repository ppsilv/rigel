
.section .vectors

_vectors:
          .long    0x000FFFF0                   /*0x00000000*//* Pilha no topo real da RAM (ajuste se seu limite for outro) */
          .long    _start                       /*0x00000004*/
          .long    0x080000      /*SvcBusError           0x0008*/
          .long    0x080006      /*Svcaddress_err        0x000C*/
          .long    0x08000C      /*SvcIllegalIns         0x0010*/
          .long    0x080012      /*SvcDiv0_handler       0x0014*/
          .long    0x080018      /*SvcChkIns             0x0018*/
          .long    0x08001E      /*SvcTrapvIns           0x001c*/
          .long    0x080024      /*SvcPrivViolation      0x0020*/
          .long    0x08002A      /*SvcTrace              0x0024*/
          .long    0x080030      /*SvcLineA              0x0028*/
          .long    0x080036      /*SvcLineF              0x002c*/
          .long    0x08003C      /*bad_exception         0x0030*/
          .long    0x080042      /*bad_exception         0x0034*/
          .long    0x080048      /*bad_exception         0x0038*/
          .long    0x08004E      /*bad_exception         0x003c*/
          .long    0x080054      /*bad_exception         0x0040*/
          .long    0x08005A      /*bad_exception         0x0044*/
          .long    0x080060      /*bad_exception         0x0048*/
          .long    0x080066      /*bad_exception         0x004c*/
          .long    0x08006C      /*bad_exception         0x0050*/
          .long    0x080072      /*bad_exception         0x0054*/
          .long    0x080078      /*bad_exception         0x0058*/
          .long    0x08007E      /*bad_exception         0x005c*/
          .long    0x080084      /*SpuriousHandler       0x0060*/
          .long    0x08008A      /*Int1Handler           0x0064*/
          .long    0x080090      /*Int2Handler           0x0068*/
          .long    0x080096      /*Int3Handler           0x006c*/
          .long    0x08009C      /*Int4Handler           0x0070*/
          .long    0x0800A2      /*Int5Handler           0x0074*/
          .long    0x0800A8      /*Int6Handler           0x0078*/
          .long    0x0800AE      /*Int7Handler           0x007c*/
          .long    0x0800B4      /*Trap0Handler          0x0080*/
          .long    0x0800BA      /*Trap1Handler          0x0084*/
          .long    0x0800C0      /*Trap2Handler          0x0088*/
          .long    0x0800C6      /*Trap3Handler          0x008c*/
          .long    0x0800CC      /*Trap4Handler          0x0090*/
          .long    0x0800D2      /*Trap5Handler          0x0094*/
          .long    0x0800D8      /*Trap6Handler          0x0098*/
          .long    0x0800DE      /*Trap7Handler          0x009c*/
          .long    0x0800E4      /*Trap8Handler          0x00a0*/
          .long    0x0800EA      /*Trap9Handler          0x00a4*/
          .long    0x0800F0      /*TrapAHandler          0x00a8*/
          .long    0x0800F6      /*TrapBHandler          0x00ac*/
          .long    0x0800FC      /*TrapCHandler          0x00b0*/
          .long    0x080102      /*TrapDHandler          0x00b4*/
          .long    0x080108      /*TrapEHandler          0x00b8*/
          .long    0x08010E      /*TrapFHandler          0x00bc*/

          .rept    207
          .long    DefaultHandler
          .endr

.section .text

.global _start
.global _delay_ms
.global printString
.global UartWriteCh

.extern duart_send_char
.extern build_jump_table
.extern sys_setramvector
.extern Int3Handler
.extern Trap1Handler
.extern sys_setramvector

.equ WR_CHAR, 0xFF912B
.equ DUART_BASE, 0x00FF9000

        .section .text.start,"ax"
        .global _start
_header:
        bra.w   _start                  /* offset 0: o Rigel salta aqui */
        .long   0x4B524E4C              /* offset 4: magic 'KRNL' */
        .long   _end                    /* offset 8: fim do kernel com .bss */
_start:
        ORI.W   #0x0700,%SR         /* Desabilita interrupções no boot */
        LEA     0x000FFFF0,%A7      /* Garante o ponteiro da pilha limpo */

        MOVE.L  #0x00000000,%D0
        MOVE.L  #0x00000000,%D1
        MOVE.L  #0x00000000,%D2
        MOVE.L  #0x00000000,%D3
        MOVE.L  #0x00000000,%D4
        MOVE.L  #0x00000000,%D5
        MOVE.L  #0x00000000,%D6
        MOVE.L  #0x00000000,%D7
        LEA.L    0x00000000,%A0
        LEA.L    0x00000000,%A1
        LEA.L    0x00000000,%A2
        LEA.L    0x00000000,%A3
        LEA.L    0x00000000,%A4
        LEA.L    0x00000000,%A5
        LEA.L    0x00000000,%A6

        MOVE.L  #3,%D1        /* espera 5 segundos */
        JSR     delay_seconds

//        MOVE.B  #'1',WR_CHAR

        /* Duart A init */
        // DUART_ACR = 0x00; (seleciona baud Set 1)
        move.b #0x00, (DUART_BASE + 0x09).l
        // DUART_SRA_CSRA = 0xCC; (Rx/Tx = 38400 baud, cristal 3.6864MHz)
        move.b #0xCC, (DUART_BASE + 0x03).l
        // DUART_CRA = 0x10; (reseta ponteiro MR -> aponta p/ MR1A)
        move.b #0x10, (DUART_BASE + 0x05).l
        // DUART_MR1A_MR2A = 0x13; (MR1A: 8 bits, sem paridade, char mode)
        move.b #0x13, (DUART_BASE + 0x01).l
        // DUART_MR1A_MR2A = 0x07; (MR2A: modo normal, 1 stop bit)
        move.b #0x07, (DUART_BASE + 0x01).l
        // DUART_CRA = 0x20; (reset receiver)
        move.b #0x20, (DUART_BASE + 0x05).l
        // DUART_CRA = 0x30; (reset transmitter)
        move.b #0x30, (DUART_BASE + 0x05).l
        // DUART_CRA = 0x05; (habilita Rx e Tx)
        move.b #0x05, (DUART_BASE + 0x05).l

//        MOVE.B  #'2',WR_CHAR
        jsr     clear_screen
        lea     bootstrap_string,%a0
        jsr     print_string

//        MOVE.B  #'3',WR_CHAR

/*
; Busca de RAM para m68k (Orion68)
; Retorna em A0 o endereço onde a RAM termina.
*/
/************************************************************************************* */
/* Find the total of SRAM */
FIND_RAM:
    MOVE.L  #0x00080000,%A0    /*; A0 = Endereço inicial da busca (Ajuste para o seu mapa)*/
    MOVE.L  #0x55AA55AA,%D3    /*; D3 = Padrão de teste (alternando 0s e 1s)*/
    MOVE.L  #0x00000400,%D2    /*; D2 = Incremento de salto (ex: 1KB)*/


.busca_loop:
    MOVE.L  (%A0),%D0          /*; Salva o dado original da memória no D0*/
    MOVE.L  %D3,(%A0)          /*; Escreve o padrão de teste na RAM*/

    CMP.L   (%A0),%D3          /*; Lê da RAM e compara com o padrão*/
    BNE.S   .fim_ram          /*; Se não for igual (não há RAM), sai do loop*/

    MOVE.L  %D0,(%A0)          /*; Restaura o dado original (é RAM válida)*/
    MOVE.L  %A0,%D1
    BSR     print_hex_32
    ADD.L   %D2,%A0            /*; Avança o ponteiro de memória*/
    BRA.S   .busca_loop       /*; Repete para o próximo bloco*/

.fim_ram:
        /*; A0 contém o endereço do fim da RAM.*/
        MOVE.L  %A0,%A4
        /*; A4 contém o endereço do fim da RAM.*/

//    MOVE.B  #'4',WR_CHAR

/********************************************/
/* Clear the entire SRAM */
        /* 1. Limpa a RAM (bss) */
        LEA     0x080000,%A0
        MOVE.L  %A4,%A1
        MOVE.L  #0x0,%D0
.ClearLoop:
        MOVE.L  %D0,(%A0)+

|    MOVE.L  %A0,%D1                    É MUITO LENTO A IMPRESSÃO DISSO
|    BSR     print_hex_32

        CMPA.L  %A0,%A1
        BHI     .ClearLoop
/*Teste se parar de precisar de teclar uma tecla depois de comentar isso é falta de dtack.*/
|        LEA     0x080400,%A0
|        MOVE.L  %A4,(%A0)

//        MOVE.B  #'5',WR_CHAR
/*Aqui estava posicionando o SP no final da memoria existente.
  mas achei desnecessário. por que o SP tem que ficar no final
  da memoria existente? NÃO TEM pode ficar no fina do primeiro
  bloco de 512k.
*/  
/*        MOVE.L  %A4,%D0
//        ANDI.L  #0xFFFFFFFC,%D0
//        MOVE.L  %D0,%A7
*/

/********************************************/
/* 2. Carrega seção .data da ROM para a RAM */
        lea     __data_load_start, %A0
        lea     __data_start, %A1
        lea     __data_end, %A2
copy_data:
        cmpa.l  %A2, %A1
        bge.s   go_ahead
        move.b  (%A0)+, (%A1)+
        bra.s   copy_data
go_ahead:

//        MOVE.B  #'6',WR_CHAR

        /*Para empilhar os parametros de uma função
          começamos do ultimo para o primeiro.
          e no retorno da função temos que somar (4 * total de parâmetros) no SP.      
        */        
        | 1. Empilha o último argumento (count)
        move.l  #50, -(%SP)            /*#254, -(%SP)*/
        | 2. Empilha o segundo argumento (target_addr)
        move.l  #DefaultHandler, -(%SP)
        | 3. Empilha o primeiro argumento (base_addr)
        move.l  #0x00080000, -(%SP)
        | Chama a função C
        BSR     build_jump_table
        | Limpa a pilha após o retorno (3 argumentos * 4 bytes = 12 bytes)
        ADDA.L  #12, %SP

//        MOVE.B  #'7',WR_CHAR

        /*INSTALLING INT2 HANDLER*/
/*NÃO PODE INSTALAR ISSO ANTES DO KERNEL CONFIGURAR TUDO SENÃO VAI CHAMAR O SCHEDULER.
        move.l  #Int2Handler, -(%SP)     | handler_addr = OS_TickISR (empilhado 1o, e' o ultimo parametro)
        move.l  #0x00080090, -(%SP)      | stub_addr = 0x00080090 (empilhado 2o, e' o 1o parametro)
        BSR     sys_setramvector         | Chama a função C
        ADDA.L  #8, %SP                  | 2 argumentos * 4 bytes = 8 (o comentario diz "3 argumentos", mas a conta certa e' 2)
*/
        move.l  #1,%D1
        BSR     delay_seconds
//        MOVE.B  #'8',WR_CHAR
/*
        |INSTALLING Int3Handler 0x00080096
        move.l  #Int3Handler, -(%SP)    | handler_addr = Int3_Handler (empilhado 1o, e' o ultimo parametro)
        move.l  #0x00080096, -(%SP)      | stub_addr = 0x00080096 (empilhado 2o, e' o 1o parametro)
        BSR     sys_setramvector         | Chama a função C
        ADDA.L  #8, %SP                  | 2 argumentos * 4 bytes = 8 (o comentario diz "3 argumentos", mas a conta certa e' 2)
*/
/*
        |INSTALLING TRAP1 HANDLER
        move.l  #Trap1Handler, -(%SP)    | handler_addr = trap1_handler (empilhado 1o, e' o ultimo parametro)
        move.l  #0x000800ba, -(%SP)      | stub_addr = 0x000800ba (empilhado 2o, e' o 1o parametro)
        BSR     sys_setramvector         | Chama a função C
        ADDA.L  #8, %SP                  | 2 argumentos * 4 bytes = 8 (o comentario diz "3 argumentos", mas a conta certa e' 2)

        move.l  #1,%D1
        BSR     delay_seconds
        MOVE.B  #'8',WR_CHAR

        |INSTALLING TRAP2 HANDLER
        move.l  #Trap2Handler, -(%SP)    | handler_addr = trap1_handler (empilhado 1o, e' o ultimo parametro)
        move.l  #0x000800c0, -(%SP)      | stub_addr = 0x000800ba (empilhado 2o, e' o 1o parametro)
        BSR     sys_setramvector         | Chama a função C
        ADDA.L  #8, %SP                  | 2 argumentos * 4 bytes = 8 (o comentario diz "3 argumentos", mas a conta certa e' 2)

        move.l  #1,%D1
        BSR     delay_seconds
        MOVE.B  #'8',WR_CHAR

        |INSTALLING TRAPC HANDLER
        move.l  #TrapCHandler, -(%SP)    | handler_addr = TrapCHandler (empilhado 1o, e' o ultimo parametro)
        move.l  #0x000800fc, -(%SP)      | stub_addr = 0x000800fc (empilhado 2o, e' o 1o parametro)
        BSR     sys_setramvector         | Chama a função C
        ADDA.L  #8, %SP                  | 2 argumentos * 4 bytes = 8 (o comentario diz "3 argumentos", mas a conta certa e' 2)
*/
//        MOVE.B  #'9',WR_CHAR
/*
        lea     interrupt_status,%a0
        jsr     print_string
        andi.w #0xF8FF,%SR
*/
        jsr     main

.dead:
        bra.s   .dead

PicoWrCh:
        LEA     0x00FF8001,%A1
        MOVE.B  %D0,(%A1)
        NOP
        NOP
        NOP
        NOP
        RTS

printString:
        MOVE.L  4(%A7),%A0     /* pega o 1º argumento da pilha (ponteiro da string) e põe em A0 */
        MOVE.L  %A0,-(%A7)     /* preserva A0 original do chamador (é caller-saved, mas por segurança) */
        ADDQ.L  #4,%A7
print_string:
        move.b  (%A0)+,%d0
        beq     .end_print_string
        jsr     duart_send_char
        /*jsr     PicoWrCh*/
        bra     print_string
.end_print_string:
        RTS

startup_delay:
        move.l  #2000,%d1
.outer_loop:
        move.l  #400,%d2
.inner_loop:
        subq.l  #1,%d2
        bne     .inner_loop
        subq.l  #1,%d1
        bne     .outer_loop
        rts

clear_screen:
    lea     .Lclr(%pc), %a0
1:  move.b  (%a0)+, %d0       /* pega o próximo byte; Z=1 se for 0 */
    beq     2f
    bsr     duart_send_char
    bra     1b
2:  rts

.Lclr:
    .byte   0x1B
    .ascii  "[2J"
    .byte   0x1B
    .asciz  "[H"        
/*
; ---------------------------------------------------------
; DELAY_1MS: Gera um atraso de aproximadamente 1 ms
; Frequência da CPU: 16 MHz (Período do clock = 62.5 ns)
; ---------------------------------------------------------
*/
_delay_ms:
    MOVE.L  %D0, -(%A7)
    MOVE.L  #1333,%D0
.loop1:
    DBRA    %D0, .loop1

    MOVE.L  (%A7)+,%D0
    RTS
/*
; ---------------------------------------------------------
; DELAY_1MS_8MHZ: Gera um atraso de aproximadamente 1 ms
; Frequência da CPU: 8 MHz (Período do clock = 125 ns)
; ---------------------------------------------------------
*/
DELAY_1MS_8MHZ:
    MOVE.L  %D0,-(%A7)
    MOVE.L  #666,%D0
.loop2:
    DBRA    %D0, .loop2
    MOVE.L  (%A7)+,%D0
    RTS
/*
; ---------------------------------------------------------
; DELAY_1MS_4MHZ: Gera um atraso de aproximadamente 1 ms
; Frequência da CPU: 4 MHz (Período do clock = 250 ns)
; ---------------------------------------------------------
*/
DELAY_1MS_4MHZ:
    MOVE.L  %D0,-(%A7)
    MOVE.L  #333,%D0
.loop3:
    DBRA    %D0, .loop3
    MOVE.L  (%A7)+,%D0
    RTS

/*
; ---------------------------------------------------------
; DELAY_SECONDS: Gera um atraso de N segundos
; Entrada: D1 = número de segundos (0 = retorna imediatamente)
; Usa _delay_ms internamente (assume 16MHz, ajuste se mudar clock)
; ---------------------------------------------------------
*/
delay_seconds:
        MOVE.L  %D0,-(%A7)
        MOVE.L  %D1,-(%A7)

        TST.L   %D1
        BEQ     .delay_seconds_done   /* se pediram 0 segundos, sai direto */

.loop_outer:
        MOVE.L  #550,%D0             /* 1000 x 1ms = 1 segundo */
.loop_inner:
        JSR     DELAY_1MS_8MHZ
        SUBQ.L  #1,%D0
        BNE     .loop_inner

        SUBQ.L  #1,%D1
        BNE     .loop_outer

.delay_seconds_done:
        MOVE.L  (%A7)+,%D1
        MOVE.L  (%A7)+,%D0
        RTS

DefaultHandler:
        rte

print_hex_32:
        movem.l %d0-%d3/%a0,-(%a7)
        moveq   #7,%d2            /* 8 dígitos hexadecimais (32 bits / 4) */

.loop:
        rol.l   #4,%d1            /* 1º: Roda 4 bits à esquerda (joga o MSB para o LSB) */
        move.l  %d1,%d3           /* 2º: Copia para o registrador de trabalho */
        and.i   #0xF,%d3          /* 3º: Isola o nibble */
        
        cmp.b   #10,%d3
        blt.s   .is_digit
        add.b   #7,%d3            /* Ajusta para 'A'-'F' */
.is_digit:
        add.b   #'0',%d3          /* Converte para ASCII */

        move.b  %d3,%d0
        bsr     duart_send_char   /* Envia o caractere atual */

        dbf     %d2,.loop

        /* Envia comando VT102 para voltar 8 colunas: ESC [ 8 D */
        move.b  #0x1B,%d0          /* ESC */
        bsr     duart_send_char
        move.b  #'[',%d0
        bsr     duart_send_char
        move.b  #'8',%d0          /* 8 posições para trás (8 dígitos) */
        bsr     duart_send_char
        move.b  #'D',%d0          /* Cursor Left (VT102) */
        bsr     duart_send_char

        movem.l (%a7)+,%d0-%d3/%a0
        rts
.section .text
.align 2
bootstrap_string:
        .byte   13, 10
        .ascii  "PDS317 - Bootstrap 2026 V1.0"
        .byte   13, 10
        .ascii  "Memory test: "
        .byte   0
passo1: .byte 13,10
        .ascii  "passo1",0
passo2: .byte 13,10
        .ascii  "passo2",0
passo3: .byte 13,10
        .ascii  "passo3",0
passo4: .byte 13,10
        .ascii  "passo4",0
passo5: .byte 13,10
        .ascii  "passo5",0

interrupt_status:
        .ascii  "Bootstrap interrupt on"
        .byte   13, 10, 0

.align 2        /* <--- CRÍTICO: Garante que o PRÓXIMO arquivo linkado comece em endereço PAR */

/*
        MEMORY ADDRESS

0x00000000      TO      0x00000002      =       RESET VECTOR
0x00000004      TO      0x0000003E      =       INTERRUPT VECTORS
0x00000040      TO      0x0000007E      =       TRAP VECTORS

*/
/*        MOVE.L %A4,0x80000*/
        /*; 2. Alinha o endereço (Garante que é múltiplo de 4)*/
        /*; Se A0 terminar em um endereço ímpar ou não alinhado a 32 bits,*/
        /*; esta operação arredonda o valor para baixo (área segura da RAM).*/
/*        MOVE.L  %A4,%D0 */
/*        ANDI.L  #0xFFFFFFFC,%D0*/    /*; Limpa os 2 bits menos significativos (alinha a 4 bytes)*/
/*        MOVE.L  %D0,%A7 */           /*; Seta o Stack Pointer (A7 / SP) com o valor seguro*/
