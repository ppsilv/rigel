/*
 * trap0.s  --  KERNEL. Handler do TRAP #0 (todas as syscalls dos apps).
 *
 * Entrada: D0 = numero da syscall (indice em sys_call_table).
 *          Os argumentos estao na pilha, exatamente como numa chamada de
 *          funcao normal (o stub do app fez  trap #0  logo depois do jsr).
 * Saida:   resultado em D0.
 *
 * 68000: o frame da excecao tem 6 bytes (SR 2 + PC 4). Se um dia usar 68010,
 * o frame tem 8 bytes: troque o 10 abaixo por 12.
 */
.text
.global trap0_handler
.extern kernel_syscall_dispatcher

trap0_handler:
        lea     10(%sp), %a0                     /* a0 -> argumentos do app: SR(2)+PC(4)+retorno do stub(4) */
        move.l  %a0, -(%sp)                      /* 2o argumento do dispatcher: long *stack_args */
        move.l  %d0, -(%sp)                      /* 1o argumento do dispatcher: trap_num */
        jsr     kernel_syscall_dispatcher
        addq.l  #8,%sp
        rte                                     /* D0 ja tem o resultado */
