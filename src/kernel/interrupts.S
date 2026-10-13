.text

.global Int3Handler

.extern ring_buf_put

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

//Está sendo tratada no ctx_switch_68000.S
//Int2Handler:

Int3Handler:
        movem.l %d0-%d7/%a0-%a6,-(%sp)

        MOVE.B  0x00FF9113,%D0  /*Lê o valor da tecla no pico2w*/

        ANDI.L  #0x000000FF,%D0  /*; Limpa a parte alta (garante o unsigned)*/
        /*MOVE.B  %D0,0x00FF8001*/   /*Ecoa no video, aqui está correto o valor da tecla*/
        MOVE.L  %D0, -(%A7)       /*; Empilha 4 bytes (mantém A7 alinhado em par)*/
        
        move.w  #0x2700, %sr  /*Disable all interrupts supervisor mode*/       
        jsr     ring_buf_put
        move.w  #0x2000, %sr  /*Enable all interrupts supervisor mode*/    
        
        ADDQ.L  #4,%A7          /*; Limpa os 4 bytes da pilha!*/

        movem.l (%sp)+,%d0-%d7/%a0-%a6
        rte    