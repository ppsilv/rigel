/*
 * crt0_app.S -- inicio de todo programa de aplicacao do Rigel.
 *
 * O kernel (SYS_SPAWN) deixa no topo da pilha: [argc][argv ...]. No primeiro
 * instante da task, SP aponta para esse bloco:  0(sp)=argc  4(sp)=argv.
 * Guarda em D2/D3 (o bss clear so' usa D0/A0/A1), zera o .bss e chama
 * main(argc, argv). Se main voltar, chama exit(codigo).
 */
        .section .text.start,"ax"
        .global _start
_start:
        move.l  (%sp),%d2               /* argc */
        move.l  4(%sp),%d3              /* argv */
        lea     __bss_start,%a0
        lea     __bss_end,%a1
        moveq   #0,%d0
1:      cmp.l   %a1,%a0
        bcc.s   2f
        move.l  %d0,(%a0)+
        bra.s   1b
2:
        move.l  %d3,-(%sp)              /* argv */
        move.l  %d2,-(%sp)              /* argc */
        jsr     main
        addq.l  #8,%sp
        move.l  %d0,-(%sp)              /* codigo de saida de main */
        jsr     exit
3:      bra.s   3b
