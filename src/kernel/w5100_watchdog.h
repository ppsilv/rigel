/*
 * w5100_watchdog.h (v3) -- thread que confere o chip W5100 e o reinicializa se ele perder o estado.
 *
 * Incluir em w5100_netif.c ANTES de  #include "w5100_netmon.h"  (assim o netmon enxerga os
 * contadores nm_hw_*). Usa hw_init(), hw_lock, cfg_mac e os registradores definidos no
 * proprio w5100_netif.c.
 *
 * v2: tira uma FOTO dos registradores do chip no primeiro instante em que hw_ok() falha
 *     (antes do reinit apagar as evidencias) e imprime a foto junto com o aviso. Assim da'
 *     para saber COMO o chip perdeu o estado:
 *       - reset do chip      : MR=00 RMSR=55 TMSR=55 SHAR=000000000000 S0_MR=00 SR=00
 *       - socket fechado     : so' SR=00 (ou CR=10), o resto intacto
 *       - barramento/escrita : valores aleatorios em alguns registradores
 *     "1o teste que falhou": 1=SR 2=S0_MR 3=RMSR 4=TMSR 5..10=SHAR[0..5], valor=o que foi lido.
 *     Tambem conta (nm_hw_glitch) as vezes em que o chip estava errado numa conferencia e
 *     voltou ao normal sozinho na seguinte, e mostra por quanto tempo ficou fora do estado.
 *
 * v3: cada bloco de mensagens e' impresso entre NM_OUT_BEGIN()/NM_OUT_END() (definidos em
 *     w5100_netif.c), para o watchdog e o netmon nao misturarem as linhas na serial/video.
 *
 * v4: para reduzir a queda de rede enquanto a causa do reset do chip nao e' achada:
 *     confere a cada 200 ms (era 1 s) e confirma duas vezes, entao reinicia o chip ~0,2-0,4 s
 *     depois da falha (era ~1,5-2,5 s). Alem disso, low_level_input() reinicia o chip na hora
 *     quando ve um quadro invalido e hw_ok() esta falso (conta em nm_hw_reinit).
 */
#ifndef W5100_WATCHDOG_H
#define W5100_WATCHDOG_H

#ifndef NM_OUT_BEGIN                       /* normalmente definido em w5100_netif.c */
#define NM_OUT_BEGIN()  do { } while (0)
#define NM_OUT_END()    do { } while (0)
#endif

#define WD_CHECK_TICKS   20         /* confere a cada ~200 ms                         */
#define WD_RETRY_TICKS   200        /* depois de um reinit, espera ~2 s             */
#define WD_LOCK_MS       2000       /* tempo maximo esperando o hw_lock             */
#define WD_STACK         3072

static volatile uint32_t nm_hw_reinit, nm_hw_reinit_fail, nm_hw_lockstuck, nm_hw_glitch;

/* qual teste do hw_ok() falhou e o valor lido (da ultima chamada que falhou) */
static volatile uint8_t hw_why, hw_why_val;
/* foto: MR RMSR TMSR SHAR[6] S0_MR S0_SR S0_CR S0_IR  + o 1o teste que falhou */
static volatile uint8_t hw_dump[13];
static volatile uint8_t wd_why1, wd_why1_val;

static void wd_dec(uint32_t v)
{
    char b[12]; int i = 11; b[i] = 0;
    if (v == 0) { kernel_puts("0"); return; }
    while (v && i > 0) { b[--i] = (char)('0' + v % 10); v /= 10; }
    kernel_puts(&b[i]);
}
static void wd_hex2(uint8_t v)
{
    static const char h[] = "0123456789ABCDEF";
    char b[4]; b[0] = h[(v >> 4) & 15]; b[1] = h[v & 15]; b[2] = ' '; b[3] = 0;
    kernel_puts(b);
}

/* 1 = o chip ainda esta como o hw_init() deixou */
static int hw_ok(void)
{
    uint8_t v;
    int i;

    v = HW_RD(S0_SR);
    if (v != SOCK_MACRAW)                { hw_why = 1; hw_why_val = v; return 0; }
    v = HW_RD(S0_MR);
    if (v != (S_MR_MACRAW | S_MR_MF))    { hw_why = 2; hw_why_val = v; return 0; }
    v = HW_RD(W_RMSR);
    if (v != 0x03)                       { hw_why = 3; hw_why_val = v; return 0; }
    v = HW_RD(W_TMSR);
    if (v != 0x03)                       { hw_why = 4; hw_why_val = v; return 0; }
    for (i = 0; i < 6; i++) {
        v = HW_RD(W_SHAR + i);
        if (v != cfg_mac[i])             { hw_why = (uint8_t)(5 + i); hw_why_val = v; return 0; }
    }
    return 1;
}

/* le os registradores SEM pegar o hw_lock (de proposito) */
static void hw_snapshot(void)
{
    int i;
    hw_dump[0]  = HW_RD(W_MR);
    hw_dump[1]  = HW_RD(W_RMSR);
    hw_dump[2]  = HW_RD(W_TMSR);
    for (i = 0; i < 6; i++) hw_dump[3 + i] = HW_RD(W_SHAR + i);
    hw_dump[9]  = HW_RD(S0_MR);
    hw_dump[10] = HW_RD(S0_SR);
    hw_dump[11] = HW_RD(S0_CR);
    hw_dump[12] = HW_RD(S0_IR);
}

/* chamar entre NM_OUT_BEGIN()/NM_OUT_END() */
static void wd_print_snapshot(void)
{
    int i;
    kernel_puts("      foto: MR=");   wd_hex2(hw_dump[0]);
    kernel_puts(" RMSR=");            wd_hex2(hw_dump[1]);
    kernel_puts(" TMSR=");            wd_hex2(hw_dump[2]);
    kernel_puts(" SHAR=");
    for (i = 0; i < 6; i++) wd_hex2(hw_dump[3 + i]);
    kernel_puts(" S0_MR=");           wd_hex2(hw_dump[9]);
    kernel_puts(" SR=");              wd_hex2(hw_dump[10]);
    kernel_puts(" CR=");              wd_hex2(hw_dump[11]);
    kernel_puts(" IR=");              wd_hex2(hw_dump[12]);
    kernel_puts(" 1o teste que falhou="); wd_dec(wd_why1);
    kernel_puts(" valor=");           wd_hex2(wd_why1_val);
    kernel_puts("\n");
}

static void wd_thread(void *arg)
{
    int bad = 0;
    uint32_t t_bad = 0;                                  /* quando o chip foi visto errado pela 1a vez */
    (void)arg;

    OS_SleepTicks(200);                                  /* deixa o boot terminar */
    if (!hw_ok()) {
        /* se falha logo apos o boot, o valor esperado esta errado: nao entra em laco de reset */
        hw_snapshot(); wd_why1 = hw_why; wd_why1_val = hw_why_val;
        NM_OUT_BEGIN();
        kernel_puts("[net] watchdog DESLIGADO: hw_ok() falhou logo apos o boot\n");
        wd_print_snapshot();
        NM_OUT_END();
        for (;;) OS_SleepTicks(6000);                    /* nao retorna (veja OS_Reap) */
    }
    //NM_OUT_BEGIN();
    //kernel_puts("[net] watchdog do chip ativo\n");
    //NM_OUT_END();

    for (;;) {
        OS_SleepTicks(WD_CHECK_TICKS);

        if (hw_ok()) {
            if (bad) {                                   /* estava errado numa conferencia e voltou sozinho */
                nm_hw_glitch++;
                NM_OUT_BEGIN();
                kernel_puts("[net] chip voltou ao normal sozinho (glitch)\n");
                wd_print_snapshot();
                NM_OUT_END();
            }
            bad = 0;
            continue;
        }
        if (++bad == 1) {                                /* foto no primeiro instante, antes de qualquer reinit */
            hw_snapshot();
            wd_why1 = hw_why; wd_why1_val = hw_why_val;
            t_bad = systemTick;
        }
        if (bad < 2) continue;                           /* confirma duas vezes: evita falso alarme */

        /* pega o lock com limite de tempo; se estiver preso, so avisa (nao mexe no chip) */
        if (sys_arch_sem_wait((sys_sem_t *)&hw_lock, WD_LOCK_MS) == SYS_ARCH_TIMEOUT) {
            if (nm_hw_lockstuck++ == 0) {
                NM_OUT_BEGIN();
                kernel_puts("[net] watchdog: hw_lock preso, nao consigo reiniciar o chip\n");
                NM_OUT_END();
            }
            continue;
        }

        if (!hw_ok()) {
            nm_hw_reinit++;
            NM_OUT_BEGIN();
            kernel_puts("[net] chip fora do estado, reinicializando\n");
            wd_print_snapshot();
            NM_OUT_END();
            if (hw_init() != 0 || !hw_ok()) {
                nm_hw_reinit_fail++;
                NM_OUT_BEGIN();
                kernel_puts("[net] reinit FALHOU\n");
                NM_OUT_END();
            } else {
                NM_OUT_BEGIN();
                kernel_puts("[net] reinit ok, chip ficou fora do estado ~");
                wd_dec((uint32_t)(systemTick - t_bad) * 10UL);
                kernel_puts(" ms\n");
                NM_OUT_END();
            }
        } else {
            nm_hw_glitch++;                              /* voltou ao normal enquanto esperava o lock */
        }
        sys_mutex_unlock(&hw_lock);

        bad = 0;
        OS_SleepTicks(WD_RETRY_TICKS);
    }
}

static void wd_start(void)
{
    if (sys_thread_new("w5100wd", wd_thread, NULL, WD_STACK, 1) < 0) {
        NM_OUT_BEGIN();
        kernel_puts("[net] watchdog nao iniciou\n");
        NM_OUT_END();
    }
}

#endif /* W5100_WATCHDOG_H */
