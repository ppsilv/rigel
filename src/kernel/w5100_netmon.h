/*
 * w5100_netmon.h -- monitor de rede do Rigel (diagnostico de "o ping parou de responder").
 *
 * E' INCLUIDO por w5100_netif.c (aplique o patch_netmon.py). Cria uma task "netmon" que:
 *   - a cada NM_CHECK_S segundos (60) confere a saude da rede e, se algo estiver errado,
 *     escreve NA HORA uma linha "ALERTA" no video dizendo O QUE esta errado;
 *   - a cada NM_PRINT_S segundos (30 min) escreve uma linha de status, com a hora (tempo ligado).
 *
 * O que e' conferido:
 *   1 task w5100rx parada (contador de voltas nao anda)  -> mostra o ESTAGIO em que ficou
 *   2 task tcpip nao responde (um aviso postado nela nao volta em 60 s)
 *   3 chip W5100: socket 0 saiu do modo MACRAW
 *   4 nenhum quadro recebido ha NM_NOFRAME_S segundos (5 min)
 *   5 envio (tx) preso dentro de low_level_output
 *   6/7 heap do lwIP >90% cheio / sem memoria (precisa de LWIP_STATS 1 no lwipopts.h)
 *
 * Estagios da task rx : 0 dormindo/entre voltas, 1 esperando o lock/lendo o chip, 2 dentro do lwIP
 *                       (nif->input), 3 lendo o quadro do chip.
 * Estagios do tx      : 0 livre, 1 esperando o lock do chip, 2 escrevendo no chip.
 *
 * Os registradores do chip sao lidos SEM pegar o hw_lock (de proposito: se o lock estiver
 * preso, o monitor continua conseguindo diagnosticar).
 */
#ifndef W5100_NETMON_H
#define W5100_NETMON_H

#include <sys/scheduler.h>
#if LWIP_STATS
#include "lwip/stats.h"
#endif

#define NM_CHECK_S     60UL        /* de quanto em quanto tempo confere                 */
#define NM_PRINT_S     1800UL      /* de quanto em quanto tempo escreve o status (30 min) */
#define NM_NOFRAME_S   300UL       /* sem quadros recebidos por tanto tempo = alerta     */
#define NM_STACK       4096

/* ---- escrita no video ---- */
static void nm_dec(uint32_t v)
{
    char b[12]; int i = 11; b[i] = 0;
    if (v == 0) { kernel_puts("0"); return; }
    while (v && i > 0) { b[--i] = (char)('0' + v % 10); v /= 10; }
    kernel_puts(&b[i]);
}
static void nm_pad2(uint32_t v) { if (v < 10) kernel_puts("0"); nm_dec(v); }
static void nm_hex2(uint8_t v)
{
    static const char h[] = "0123456789ABCDEF";
    char b[3]; b[0] = h[(v >> 4) & 15]; b[1] = h[v & 15]; b[2] = 0;
    kernel_puts(b);
}
static void nm_stamp(void)          /* "[net HH:MM:SS] " = tempo desde o boot */
{
    uint32_t s = systemTick / 100UL;
    kernel_puts("[net ");
    nm_pad2(s / 3600UL); kernel_puts(":");
    nm_pad2((s / 60UL) % 60UL); kernel_puts(":");
    nm_pad2(s % 60UL);
    kernel_puts("] ");
}

/* ---- envio com contagem e estagio (usado como netif->linkoutput) ---- */
static err_t nm_linkoutput(struct netif *n, struct pbuf *p)
{
    err_t e;
    nm_tx_stage = 1;
    e = low_level_output_i(n, p);
    nm_tx_stage = 0;
    if (e == ERR_OK) nm_tx_frames++; else nm_tx_errors++;
    return e;
}

/* ---- teste de vida da task tcpip ---- */
static volatile uint32_t nm_posts, nm_acks;
static void nm_tcpip_cb(void *ctx) { (void)ctx; nm_acks++; }

static void nm_print_status(int tcpip_ok)
{
    nm_stamp();
    kernel_puts("rx="); nm_dec(nm_rx_frames);
    kernel_puts(" tx="); nm_dec(nm_tx_frames);
    kernel_puts(" semmem="); nm_dec(nm_rx_nomem);
    kernel_puts(" txerr="); nm_dec(nm_tx_errors);
    kernel_puts(" sr="); nm_hex2(HW_RD(S0_SR));
    kernel_puts(" rsr="); nm_dec(rd16(S0_RX_RSR));
    kernel_puts(tcpip_ok ? " tcpip=ok\n" : " tcpip=PARADO\n");
#if LWIP_STATS && MEM_STATS
    kernel_puts("      heap usado="); nm_dec((uint32_t)lwip_stats.mem.used);
    kernel_puts(" max="); nm_dec((uint32_t)lwip_stats.mem.max);
    kernel_puts(" de "); nm_dec((uint32_t)lwip_stats.mem.avail);
    kernel_puts(" err="); nm_dec((uint32_t)lwip_stats.mem.err);
#if ICMP_STATS
    kernel_puts(" icmp rx="); nm_dec((uint32_t)lwip_stats.icmp.recv);
    kernel_puts(" tx="); nm_dec((uint32_t)lwip_stats.icmp.xmit);
#endif
    kernel_puts("\n");
#endif
}

static void nm_thread(void *arg)
{
    uint32_t last_loops  = nm_rx_loops;
    uint32_t last_frames = nm_rx_frames;
    uint32_t last_txsum  = nm_tx_frames + nm_tx_errors;
    uint32_t t_frame     = systemTick;
    uint32_t t_print     = systemTick;
    uint32_t tx_stuck    = 0;
    uint32_t mask        = 0, nm, txsum;
    uint8_t  sr;
#if LWIP_STATS && MEM_STATS
    uint32_t last_memerr = (uint32_t)lwip_stats.mem.err;
#endif
    (void)arg;

    nm_stamp(); kernel_puts("monitor de rede iniciado\n");

    for (;;) {
        OS_SleepTicks(NM_CHECK_S * 100UL);
        nm = 0;

        /* 1: task rx parada */
        if (nm_rx_loops == last_loops) nm |= 1UL;
        last_loops = nm_rx_loops;

        /* 2: task tcpip (so' posta um novo aviso quando o anterior ja voltou) */
        if (nm_acks != nm_posts)                          nm |= 2UL;
        else if (tcpip_try_callback(nm_tcpip_cb, NULL) == ERR_OK) nm_posts++;
        else                                              nm |= 2UL;      /* caixa de mensagens cheia */

        /* 3: chip */
        sr = HW_RD(S0_SR);
        if (sr != SOCK_MACRAW) nm |= 4UL;

        /* 4: nenhum quadro recebido */
        if (nm_rx_frames != last_frames) { last_frames = nm_rx_frames; t_frame = systemTick; }
        else if ((uint32_t)(systemTick - t_frame) > NM_NOFRAME_S * 100UL) nm |= 8UL;

        /* 5: tx preso (duas conferencias seguidas dentro do envio, sem nenhum envio concluido) */
        txsum = nm_tx_frames + nm_tx_errors;
        if (nm_tx_stage != 0 && txsum == last_txsum) tx_stuck++; else tx_stuck = 0;
        last_txsum = txsum;
        if (tx_stuck >= 2) nm |= 16UL;

#if LWIP_STATS && MEM_STATS
        /* 6/7: heap do lwIP */
        if ((uint32_t)lwip_stats.mem.used * 10UL > (uint32_t)lwip_stats.mem.avail * 9UL) nm |= 32UL;
        if ((uint32_t)lwip_stats.mem.err != last_memerr) {
            last_memerr = (uint32_t)lwip_stats.mem.err;
            nm |= 64UL;
        }
#endif

        if (nm & ~mask) {                                 /* apareceu um problema novo */
            if ((nm & 1UL) && !(mask & 1UL))  { nm_stamp(); kernel_puts("ALERTA: task w5100rx PARADA, estagio="); nm_dec(nm_rx_stage); kernel_puts("\n"); }
            if ((nm & 2UL) && !(mask & 2UL))  { nm_stamp(); kernel_puts("ALERTA: task tcpip nao responde (parada ou caixa cheia)\n"); }
            if ((nm & 4UL) && !(mask & 4UL))  { nm_stamp(); kernel_puts("ALERTA: chip saiu de MACRAW, sr="); nm_hex2(sr); kernel_puts("\n"); }
            if ((nm & 8UL) && !(mask & 8UL))  { nm_stamp(); kernel_puts("ALERTA: nenhum quadro recebido ha 5 min, rsr="); nm_dec(rd16(S0_RX_RSR)); kernel_puts("\n"); }
            if ((nm & 16UL) && !(mask & 16UL)){ nm_stamp(); kernel_puts("ALERTA: envio (tx) preso, estagio="); nm_dec(nm_tx_stage); kernel_puts("\n"); }
#if LWIP_STATS && MEM_STATS
            if ((nm & 32UL) && !(mask & 32UL)){ nm_stamp(); kernel_puts("ALERTA: heap do lwIP com mais de 90% de uso\n"); }
            if ((nm & 64UL) && !(mask & 64UL)){ nm_stamp(); kernel_puts("ALERTA: heap do lwIP ficou sem memoria\n"); }
#endif
            nm_print_status(!(nm & 2UL));
            t_print = systemTick + NM_PRINT_S * 100UL;
        } else if (nm == 0 && mask != 0) {
            nm_stamp(); kernel_puts("rede normal novamente\n");
        }
        mask = nm;

        if ((int32_t)(systemTick - t_print) >= 0) {       /* status periodico */
            nm_print_status(!(nm & 2UL));
            t_print = systemTick + NM_PRINT_S * 100UL;
        }
    }
}

static void nm_start(void)
{
    sys_thread_new("netmon", nm_thread, NULL, NM_STACK, 1);
}

#endif /* W5100_NETMON_H */
