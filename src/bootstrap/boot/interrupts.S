.text

.global Int2Handler
.global Int3Handler

.extern duart_putc

/*
+--------------------+-----------------+--------------------------+--------------------------------------------------------+
|Nível de Interrupção| Prioridade      | Uso Típico no 68K        |    Por que essa posição?                               |
+--------------------+-----------------+--------------------------+--------------------------------------------------------+
|Nível 7             | Altíssima (NMI) | Botão de Abort           |    Não pode ser desabilitada.                          |
|                    |                 | Queda de Energia         |                                                        | 
+--------------------+-----------------+--------------------------+--------------------------------------------------------+
|Nível 5 ou 6        | Alta            | Controladora de Disquete |    O fluxo de dados do disco não pode esperar,         |
|                    |                 | (WD2793) / Rede          |    ou o setor passa e o dado é perdido.                |
+--------------------+-----------------+--------------------------+--------------------------------------------------------+
|Nível 3 ou 4        | Média           | Porta Serial / UART      |    A UART tem buffers pequenos (FIFO). Precisa         |
|                    |                 | (16C554 / Z8530)         |    ser atendida rápido para não dar Overflow.          |
+--------------------+-----------------+--------------------------+--------------------------------------------------------+
|Nível 1 ou 2        | Baixa           | System Tick              |    Se o Tick atrasar alguns microssegundos para dar    |
|                    |                 | (Timer de 5ms/10ms)      |    passagem à Serial,ninguém morre,nenhum dado perdido.|
+--------------------+-----------------+--------------------------+--------------------------------------------------------+
*/


Int2Handler:
    /* Como mexe em registrador para somar na RAM, salva o D0 e A0 */
    movem.l %d0-%d7/%a0-%a6,-(%sp)

    /* Incrementa a variável de 32 bits do C */
    move.l  #systemTick,%a0
        addq.l  #1,(%a0)

    addq.w  #1,tick_count       
    cmpi.w  #400,tick_count     
    beq.s   .action_100
    cmpi.w  #800,tick_count     
    beq.s   .action_200
    movem.l (%sp)+,%d0-%d7/%a0-%a6
    rte

.action_100:
    move.b  #0x08,0xFF901D        
    movem.l (%sp)+,%d0-%d7/%a0-%a6
    rte

.action_200:
    move.b  #0x08,0xFF901F        
    clr.w   tick_count   
//    move.w  #0x2700, %sr  /*Desable all interrupts supervisor mode*/       
//    jsr     do_save2
//    move.w  #0x2000, %sr  /*Enable all interrupts supervisor mode*/    
    movem.l (%sp)+,%d0-%d7/%a0-%a6
    rte

Int3Handler:
        movem.l %d0-%d7/%a0-%a6,-(%sp)

        MOVE.B  0x00FF9113,%D0

        ANDI.L  #0x000000FF,%D0  /*; Limpa a parte alta (garante o unsigned)*/
        MOVE.L  %D0, -(%A7)       /*; Empilha 4 bytes (mantém A7 alinhado em par)*/
        
        move.w  #0x2700, %sr  /*Desable all interrupts supervisor mode*/       
        jsr     ring_buf_put
        move.w  #0x2000, %sr  /*Enable all interrupts supervisor mode*/    
        
        ADDQ.L  #4,%A7          /*; Limpa os 4 bytes da pilha!*/

        movem.l (%sp)+,%d0-%d7/%a0-%a6
        rte    