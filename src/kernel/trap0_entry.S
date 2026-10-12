/*
 * trap0_entry.S -- KERNEL. Entrada REAL do TRAP #0.
 *
 * O vetor 32 (TRAP #0) deve apontar para ESTE simbolo (trap0_entry), e nao
 * para trap0_handler. trap0_handler e' uma funcao C normal: termina com
 * "rts" e espera o numero em sp@(4). A CPU, porem, entrou por um TRAP e
 * precisa sair com "rte". Este codigo faz a ponte:
 *
 *   entrada (68000, tarefa em modo supervisor, mesma pilha):
 *       SP -> [ SR(2) ][ PC(4) ][ retorno do stub(4) ][ a1 ][ a2 ] ... [ a7 ]
 *       D0 =  numero da syscall
 *   chama:  trap0_handler(num, a1, a2, a3, a4, a5, a6, a7)   (ABI C)
 *   saida:  D0 = resultado, rte
 *
 * Nao salva registradores: o stub do app e' uma chamada de funcao, entao
 * d0/d1/a0/a1 podem ser destruidos; d2-d7/a2-a6 o proprio C preserva.
 * Se um dia usar 68010 (frame de 8 bytes), troque o 10 por 12.
 */
        .text
        .global trap0_entry
        .extern trap0_handler

trap0_entry:
        lea     10(%sp),%a0             /* a0 -> primeiro argumento do app */
        move.l  24(%a0),-(%sp)          /* a7 */
        move.l  20(%a0),-(%sp)          /* a6 */
        move.l  16(%a0),-(%sp)          /* a5 */
        move.l  12(%a0),-(%sp)          /* a4 */
        move.l   8(%a0),-(%sp)          /* a3 */
        move.l   4(%a0),-(%sp)          /* a2 */
        move.l   (%a0),-(%sp)           /* a1 */
        move.l  %d0,-(%sp)              /* num */
        jsr     trap0_handler
        lea     32(%sp),%sp             /* descarta os 8 argumentos */
        rte                             /* D0 = resultado */
        