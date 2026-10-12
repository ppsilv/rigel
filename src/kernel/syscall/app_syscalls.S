/*
 * app_syscalls.S  --  APLICACAO. Liga as funcoes de baixo nivel da newlib
 * ao kernel por TRAP #0. Linke este arquivo ANTES de -lc.
 *
 * Os argumentos ja estao na pilha (C normal): o stub so' poe o numero da
 * syscall em D0 e dispara o trap. O resultado volta em D0.
 * Os numeros sao a POSICAO na sys_call_table do kernel (trap_table.c).
 */
        .macro  SYSCALL name, num
        .global \name
\name:
        move.l  #\num,%d0
        trap    #0
        rts
        .endm

        .text
        SYSCALL _exit,    0     /* sys_exit   */
        SYSCALL open,     4     /* sys_open   */
        SYSCALL close,    5     /* sys_close  */
        SYSCALL read,     6     /* sys_read   */
        SYSCALL write,    7     /* sys_write  */
        SYSCALL lseek,    9     /* sys_lseek  */
        SYSCALL getpid,  68     /* sys_getpid */
