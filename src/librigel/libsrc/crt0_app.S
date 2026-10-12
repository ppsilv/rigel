/*
 * crt0_app.S -- inicio de todo programa de aplicacao do Rigel.
 * O loader cria a task com a pilha ja pronta, entao aqui NAO se mexe no SP.
 * Zera o .bss e chama main(0, 0). Se main voltar, chama exit().
 */
        .section .text.start,"ax"
        .global _start
_start:
        lea     __bss_start,%a0
        lea     __bss_end,%a1
        moveq   #0,%d0
1:      cmp.l   %a1,%a0
        bcc.s   2f
        move.l  %d0,(%a0)+
        bra.s   1b
2:
        clr.l   -(%sp)                  /* argv */
        clr.l   -(%sp)                  /* argc */
        jsr     main
        addq.l  #8,%sp
        move.l  %d0,-(%sp)              /* codigo de saida de main */
        jsr     exit
3:      bra.s   3b
