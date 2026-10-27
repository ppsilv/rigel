|
| ctx_switch_68000.s -- troca de contexto para CPU 68000 PURO
|
| Frame de excecao do 68000 (SEM format/vector word):
|   SP+0: SR   (2 bytes)
|   SP+2: PC   (4 bytes)
|   -- total 6 bytes --
|
| Use este arquivo se seu alvo de build for o 68000. Para 68010,
| use ctx_switch_68010.s em vez deste (nao linke os dois juntos).
|

.global OS_StartHighRdy
.global OS_TickISR
.global OS_Yield

.extern current_task
.extern OS_Schedule
.extern OS_AccountTick

.extern systemTick
.extern tick_count
.extern tick_uptime

OS_StartHighRdy:
    move.l  current_task,%A0       | %A0 = ponteiro pro TCB atual
    move.l  (%A0), %A7               | %A7 (SP) = tcb->sp (offset 0 da struct)
    movem.l (%A7)+, %D0-%D7/%A0-%A6     | restaura %D0-%D7, %A0-%A6
    rte                            | restaura PC e SR (6 bytes), salta pra tarefa

|OS_TickISROLD:
|    |movem.l %D0-%D7/%A0-%A6, -(%A7)     | salva contexto da tarefa interrompida
|    |move.l  current_task, %A0
|    move.l  %A7, (%A0)               | tcb->sp = %A7
|
|    jsr     OS_Schedule            | escolhe a proxima tarefa, atualiza current_task
|
|    move.l  current_task, %A0
|    move.l  (%A0), %A7               | %A7 = tcb->sp da tarefa escolhida
|    movem.l (%A7)+, %D0-%D7/%A0-%A6     | restaura contexto dela
|    rte                            | volta a executar de onde ela parou

OS_TickISR:
        ori.w   #0x0700,%sr
|        move.b  #0x08,0xFF901D
        movem.l %d0-%d7/%a0-%a6,-(%sp)

        move.l  #systemTick,%a0
        addq.l  #1,(%a0)
        
.continua:
        addq.w  #1,tick_count
        cmpi.w  #50,tick_count
        beq.s   .action_100
        cmpi.w  #100,tick_count
        beq.s   .action_200
        bra.s   .do_schedule

.action_100:
        move.b  #0x08,0xFF901D
        bra.s   .do_schedule

.action_200:
        move.b  #0x08,0xFF901F
        clr.w   tick_count
        | cai direto pra .do_schedule

.do_schedule:
        move.l  current_task,%a0
        move.l  %a0, %d0        | copia pra D0
        tstl    %d0             | testa D0
        beq.s   .notask         
        move.l  %sp,(%a0)              | tcb->sp = pilha da tarefa que estava rodando

        jsr     OS_AccountTick         | current_task->ticks++ (coluna TICKS do ps)
        jsr     OS_Schedule            | decide a proxima tarefa, atualiza current_task

        move.l  current_task,%a0
        move.l  (%a0),%sp              | pilha vira a da tarefa escolhida
.notask:
        movem.l (%sp)+,%d0-%d7/%a0-%a6
|        move.b  #0x08,0xFF901F
        rte     /*rte devolve automáticamente a mascara de interrupção anterior para o %SR*/
*/
|
| OS_Yield -- a task atual larga a CPU AGORA (sem esperar o tick).
|
| Quem chamou fez "jsr OS_Yield", entao o endereco de retorno (4 bytes) ja esta na pilha.
| Empilhando o SR por cima, a pilha fica [SR][PC] -- exatamente o frame que o 68000 monta numa
| excecao. Dai em diante e' o mesmo caminho do OS_TickISR: salva D0-D7/A0-A6, guarda o SP no TCB,
| chama OS_Schedule, carrega o SP da task escolhida, restaura e faz rte. Quando esta task for
| escolhida de novo, o rte volta pra instrucao logo depois do "jsr OS_Yield", com o SR de antes.
|
| So' para 68000 (frame sem format word). Chame sempre de dentro de uma task, em modo supervisor.
|
OS_Yield:
        move.w  %sr,-(%sp)              | completa o frame: [SR][PC de retorno]
        ori.w   #0x0700,%sr             | o tick nao pode entrar no meio da troca
        movem.l %d0-%d7/%a0-%a6,-(%sp)

        move.l  current_task,%a0
        move.l  %a0,%d0
        tstl    %d0
        beq.s   .yield_out              | scheduler ainda nao iniciou: so' volta

        move.l  %sp,(%a0)               | tcb->sp = pilha desta task
        jsr     OS_Schedule             | escolhe a proxima (acorda quem dormia), atualiza current_task
        move.l  current_task,%a0
        move.l  (%a0),%sp               | pilha vira a da task escolhida
.yield_out:
        movem.l (%sp)+,%d0-%d7/%a0-%a6
        rte
