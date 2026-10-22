/*
 * w5100_watchdog.h -- thread que confere o chip W5100 e o reinicializa se ele perder o estado.
 *
 * Incluir em w5100_netif.c DEPOIS do rx_thread e ANTES de  #include "w5100_netmon.h"
 * (assim o netmon enxerga os contadores nm_hw_*). Usa hw_init(), hw_lock, cfg_mac e os
 * registradores definidos no proprio w5100_netif.c.
 */
#ifndef W5100_WATCHDOG_H
#define W5100_WATCHDOG_H

#define WD_CHECK_TICKS   100        /* confere a cada ~1 s                         */
#define WD_RETRY_TICKS   500        /* depois de um reinit, espera ~5 s             */
#define WD_LOCK_MS       2000       /* tempo maximo esperando o hw_lock             */
#define WD_STACK         3072

static volatile uint32_t nm_hw_reinit, nm_hw_reinit_fail, nm_hw_lockstuck;

/* 1 = o chip ainda esta como o hw_init() deixou */
static int hw_ok(void)
{
    int i;
    if (HW_RD(S0_SR)  != SOCK_MACRAW)                    return 0;
    if (HW_RD(S0_MR)  != (S_MR_MACRAW | S_MR_MF))        return 0;
    if (HW_RD(W_RMSR) != 0x03 || HW_RD(W_TMSR) != 0x03)  return 0;
    for (i = 0; i < 6; i++)
        if (HW_RD(W_SHAR + i) != cfg_mac[i])             return 0;
    return 1;
}

static void wd_thread(void *arg)
{
    int bad = 0;
    (void)arg;

    OS_SleepTicks(200);                                  /* deixa o boot terminar */
    if (!hw_ok()) {
        /* se falha logo apos o boot, o valor esperado esta errado: nao entra em laco de reset */
        kernel_puts("[net] watchdog DESLIGADO: hw_ok() falhou logo apos o boot\n");
        for (;;) OS_SleepTicks(6000);                    /* nao retorna (veja OS_Reap) */
    }
    kernel_puts("[net] watchdog do chip ativo\n");

    for (;;) {
        OS_SleepTicks(WD_CHECK_TICKS);

        if (hw_ok()) { bad = 0; continue; }
        if (++bad < 2) continue;                         /* confirma duas vezes: evita falso alarme */

        /* pega o lock com limite de tempo; se estiver preso, so avisa (nao mexe no chip) */
        if (sys_arch_sem_wait((sys_sem_t *)&hw_lock, WD_LOCK_MS) == SYS_ARCH_TIMEOUT) {
            if (nm_hw_lockstuck++ == 0)
                kernel_puts("[net] watchdog: hw_lock preso, nao consigo reiniciar o chip\n");
            continue;
        }

        if (!hw_ok()) {
            nm_hw_reinit++;
            kernel_puts("[net] chip fora do estado, reinicializando\n");
            if (hw_init() != 0 || !hw_ok()) {
                nm_hw_reinit_fail++;
                kernel_puts("[net] reinit FALHOU\n");
            } else {
                kernel_puts("[net] reinit ok\n");
            }
        }
        sys_mutex_unlock(&hw_lock);

        bad = 0;
        OS_SleepTicks(WD_RETRY_TICKS);
    }
}

static void wd_start(void)
{
    if (sys_thread_new("w5100wd", wd_thread, NULL, WD_STACK, 1) < 0)
        kernel_puts("[net] watchdog nao iniciou\n");
}

#endif /* W5100_WATCHDOG_H */
