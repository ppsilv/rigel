/* crt0.S -- startup comum: kernel (0x82000) ou app (slot).
 *   -DBUILD_KERNEL : dono da maquina; poe o SP, mascara IRQ, nunca retorna
 *   -DBUILD_APP    : chamado pelo kernel; SP ja e' da task; main retorna -> exit()
 */
        .section .text.start,"ax"
        .global _start
        .type   _start,@function
_start:
#ifdef BUILD_KERNEL
        move.w  #0x2700,%sr             /* IRQ mascaradas ate o kernel iniciar */
        lea     _stack_top,%sp          /* definido no kernel.ld */
#endif
        /* zera .bss (alinhado a 4 pelo linker script) */
        lea     __bss_start,%a0
        lea     __bss_end,%a1
        moveq   #0,%d0
1:      cmp.l   %a1,%a0
        bcc.s   2f
        move.l  %d0,(%a0)+
        bra.s   1b
2:
        /* main(argc=0, argv=0) */
        clr.l   -(%sp)
        clr.l   -(%sp)
        jsr     main
        addq.l  #8,%sp

#ifdef BUILD_KERNEL
        move.w  #0x2700,%sr             /* main nao devia voltar */
3:      stop    #0x2700
        bra.s   3b
#else
        move.l  %d0,-(%sp)              /* codigo de saida de main */
        jsr     exit
4:      bra.s   4b
#endif