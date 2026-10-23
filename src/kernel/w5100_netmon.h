/*
 * w5100_netmon.h (v3) -- monitor de rede do Rigel (diagnostico de "o ping parou de responder").
 *
 * E' INCLUIDO por w5100_netif.c. Cria uma task "netmon" que:
 *   - a cada NM_CHECK_S segundos (5) confere a saude da rede e, se algo estiver errado,
 *     escreve NA HORA uma linha "ALERTA" dizendo O QUE esta errado;
 *   - a cada NM_PRINT_S segundos (30 min) escreve uma linha de status, com a hora (tempo ligado).
 * Tudo sai no video e na serial (o w5100_netif.c redireciona kernel_puts para nm_puts).
 *
 * O que e' conferido:
 *   1 task w5100rx parada (contador de voltas nao anda)  -> mostra o ESTAGIO em que ficou
 *   2 task tcpip nao responde (um aviso postado nela nao volta)
 *   3 chip W5100: socket 0 saiu do modo MACRAW
 *   4 nenhum quadro recebido ha NM_NOFRAME_S segundos (5 min)
 *   5 envio (tx) preso dentro de low_level_output
 *   6/7 heap do lwIP >90% cheio / sem memoria (precisa de LWIP_STATS 1 no lwipopts.h)
 *   8 leitura instavel do chip (RSR/FSR)
 *   9 quadro com tamanho invalido no chip (mostra a foto tirada na hora, nm_g_*)
 *
 * Estagios da task rx : 0 dormindo/entre voltas, 1 esperando o lock / lendo o chip,
 *                       2 dentro do lwIP (nif->input), 3 lendo RSR, 4 lendo o tamanho do quadro,
 *                       5 em pbuf_alloc, 6 copiando o quadro do chip, 7 avisando o chip (wr16/RECV)
 * Estagios do tx      : 0 livre, 1 esperando o lock do chip, 2 escrevendo no chip.
 *
 * Os registradores do chip sao lidos SEM pegar o hw_lock (de proposito: se o lock estiver
 * preso, o monitor continua conseguindo diagnosticar).
 *
 * v3: NM_CHECK_S passou de 60 para 5 s. Com 60 s o "problema durou" saia sempre 60 s (era o
 *     periodo de amostragem, nao a duracao real). Alertas 8 e 9 sao EVENTOS (o contador
 *     mudou), nao tem duracao. A duracao real do chip fora do estado vem do watchdog
 *     ("reinit ok, chip ficou fora do estado ~X ms").
 */
#ifndef W5100_NETMON_H
#define W5100_NETMON_H

#include <sys/scheduler.h>
#if LWIP_STATS
#include "lwip/stats.h"
#endif

#define NM_CHECK_S     5UL         /* de quanto em quanto tempo confere                 */
#define NM_PRINT_S     1800UL      /* de quanto em quanto tempo escreve o status (30 min) */
#define NM_NOFRAME_S   300UL       /* sem quadros recebidos por tanto tempo = alerta     */
#define NM_TXSTUCK_N   2           /* conferencias seguidas com o tx preso = alerta      */
#define NM_STACK       4096

#ifndef NM_OUT_BEGIN                       /* normalmente definido em w5100_netif.c */
#define NM_OUT_BEGIN()  do { } while (0)
#define NM_OUT_END()    do { } while (0)
#endif

extern volatile uint32_t rigel_assert_count;
extern char rigel_assert_msg[];

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


/* ---- diagnostico detalhado (so' no alerta) ---- */
static void nm_print_regs(void)
{
    kernel_puts("      chip: sr="); nm_hex2(HW_RD(S0_SR));
    kernel_puts(" cr="); nm_hex2(HW_RD(S0_CR));
    kernel_puts(" ir="); nm_hex2(HW_RD(S0_IR));
    kernel_puts(" rsr="); nm_dec(rd16(S0_RX_RSR));
    kernel_puts(" rd="); nm_dec(rd16(S0_RX_RD));
    kernel_puts(" txfsr="); nm_dec(rd16(S0_TX_FSR));
    kernel_puts("\n");
}

/* a foto tirada por low_level_input() no instante em que viu o quadro invalido */
static void nm_print_garbage(void)
{
    int k;
    kernel_puts("      lixo: len="); nm_dec(nm_g_len);
    kernel_puts(" len2="); nm_dec(nm_g_len2);
    kernel_puts(" rsr="); nm_dec(nm_g_rsr);
    kernel_puts(" ptr="); nm_dec(nm_g_ptr);
    kernel_puts(" hdr=");
    for (k = 0; k < 8; k++) { nm_hex2(nm_g_hdr[k]); kernel_puts(" "); }
    kernel_puts("\n      no instante: sr="); nm_hex2(nm_g_reg[0]);
    kernel_puts(" s0mr="); nm_hex2(nm_g_reg[1]);
    kernel_puts(" rmsr="); nm_hex2(nm_g_reg[2]);
    kernel_puts(" shar0="); nm_hex2(nm_g_reg[3]);
    kernel_puts(" rsrmax="); nm_dec(nm_rsr_max);
    kernel_puts("\n");
}

static void nm_print_rx_task(void)
{
    const tcb_t *t = (const tcb_t *)nm_rx_tcb;
    kernel_puts("      task rx: ");
    if (t == NULL) { kernel_puts("tcb desconhecido\n"); return; }
    kernel_puts("estado="); nm_dec((uint32_t)t->state);
    kernel_puts("(1 pronta,2 rodando,3 bloqueada,4 morta) dormindo="); nm_dec((uint32_t)t->sleeping);
    if (t->sleeping) {
        int32_t d = (int32_t)(systemTick - t->wake_tick);
        kernel_puts(" devia ter acordado ha "); nm_dec(d > 0 ? (uint32_t)d : 0); kernel_puts(" ticks");
    }
    kernel_puts(" estagio="); nm_dec(nm_rx_stage);
    kernel_puts("\n");
}

static void nm_print_status(int tcpip_ok)
{
    nm_stamp();
    kernel_puts("rx="); nm_dec(nm_rx_frames);
    kernel_puts(" tx="); nm_dec(nm_tx_frames);
    kernel_puts(" semmem="); nm_dec(nm_rx_nomem);
    kernel_puts(" txerr="); nm_dec(nm_tx_errors);
    kernel_puts(" inst="); nm_dec(nm_unstable);
    kernel_puts(" lixo="); nm_dec(nm_rx_garbage);
    kernel_puts(" sr="); nm_hex2(HW_RD(S0_SR));
    kernel_puts(" rsr="); nm_dec(rd16(S0_RX_RSR));
    kernel_puts(tcpip_ok ? " tcpip=ok\n" : " tcpip=PARADO\n");

    kernel_puts("      reinit="); nm_dec(nm_hw_reinit);
    kernel_puts(" falhas="); nm_dec(nm_hw_reinit_fail);
    kernel_puts(" lockpreso="); nm_dec(nm_hw_lockstuck);
    kernel_puts(" glitch="); nm_dec(nm_hw_glitch);
    kernel_puts(" rsrmax="); nm_dec(nm_rsr_max);
    nm_rsr_max = 0;                                      /* o maximo vale para a janela entre dois status */
    if (rigel_assert_count) {
        kernel_puts(" ASSERT x"); nm_dec(rigel_assert_count);
        kernel_puts(" "); kernel_puts(rigel_assert_msg);
    }
    kernel_puts("\n");

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
    uint32_t last_unst   = nm_unstable, last_garb = nm_rx_garbage;
    uint32_t t_alert     = 0;                  /* quando o problema atual foi notado */
    uint32_t mask        = 0, nm, txsum;
    uint8_t  sr;
#if LWIP_STATS && MEM_STATS
    uint32_t last_memerr = (uint32_t)lwip_stats.mem.err;
#endif
    (void)arg;

    NM_OUT_BEGIN();
    nm_stamp(); kernel_puts("monitor de rede iniciado\n");
    NM_OUT_END();

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

        /* 5: tx preso (conferencias seguidas dentro do envio, sem nenhum envio concluido) */
        txsum = nm_tx_frames + nm_tx_errors;
        if (nm_tx_stage != 0 && txsum == last_txsum) tx_stuck++; else tx_stuck = 0;
        last_txsum = txsum;
        if (tx_stuck >= NM_TXSTUCK_N) nm |= 16UL;

#if LWIP_STATS && MEM_STATS
        /* 6/7: heap do lwIP */
        if ((uint32_t)lwip_stats.mem.used * 10UL > (uint32_t)lwip_stats.mem.avail * 9UL) nm |= 32UL;
        if ((uint32_t)lwip_stats.mem.err != last_memerr) {
            last_memerr = (uint32_t)lwip_stats.mem.err;
            nm |= 64UL;
        }
#endif

        /* 8/9: leituras instaveis do chip / quadros com tamanho invalido (EVENTOS: contador mudou) */
        if (nm_unstable != last_unst) { last_unst = nm_unstable; nm |= 128UL; }
        if (nm_rx_garbage != last_garb) { last_garb = nm_rx_garbage; nm |= 256UL; }

        if (nm & ~mask) {                                 /* apareceu um problema novo */
            if (mask == 0) t_alert = systemTick;
            NM_OUT_BEGIN();
            if ((nm & 1UL) && !(mask & 1UL))  { nm_stamp(); kernel_puts("ALERTA: task w5100rx PARADA, estagio="); nm_dec(nm_rx_stage); kernel_puts("\n"); nm_print_rx_task(); nm_print_regs(); }
            if ((nm & 2UL) && !(mask & 2UL))  { nm_stamp(); kernel_puts("ALERTA: task tcpip nao responde (parada ou caixa cheia)\n"); }
            if ((nm & 4UL) && !(mask & 4UL))  { nm_stamp(); kernel_puts("ALERTA: chip saiu de MACRAW, sr="); nm_hex2(sr); kernel_puts("\n"); nm_print_regs(); }
            if ((nm & 8UL) && !(mask & 8UL))  { nm_stamp(); kernel_puts("ALERTA: nenhum quadro recebido ha 5 min, rsr="); nm_dec(rd16(S0_RX_RSR)); kernel_puts("\n"); }
            if ((nm & 16UL) && !(mask & 16UL)){ nm_stamp(); kernel_puts("ALERTA: envio (tx) preso, estagio="); nm_dec(nm_tx_stage); kernel_puts("\n"); }
            if ((nm & 128UL) && !(mask & 128UL)){ nm_stamp(); kernel_puts("ALERTA: leitura instavel do chip (RSR/FSR), total="); nm_dec(nm_unstable); kernel_puts("\n"); nm_print_regs(); }
            if ((nm & 256UL) && !(mask & 256UL)){ nm_stamp(); kernel_puts("ALERTA: quadro com tamanho invalido no chip, total="); nm_dec(nm_rx_garbage); kernel_puts("\n"); nm_print_garbage(); nm_print_regs(); }
#if LWIP_STATS && MEM_STATS
            if ((nm & 32UL) && !(mask & 32UL)){ nm_stamp(); kernel_puts("ALERTA: heap do lwIP com mais de 90% de uso\n"); }
            if ((nm & 64UL) && !(mask & 64UL)){ nm_stamp(); kernel_puts("ALERTA: heap do lwIP ficou sem memoria\n"); }
#endif
            nm_print_status(!(nm & 2UL));
            NM_OUT_END();
            t_print = systemTick + NM_PRINT_S * 100UL;
        } else if (nm == 0 && mask != 0) {
            NM_OUT_BEGIN();
            nm_stamp();
            if (mask & ~(128UL | 256UL)) {                /* houve problema que dura: tem duracao */
                kernel_puts("rede normal novamente (problema durou ~");
                nm_dec((systemTick - t_alert) / 100UL);
                kernel_puts(" s, resolucao "); nm_dec(NM_CHECK_S); kernel_puts(" s)\n");
            } else {                                      /* so' contadores de evento mudaram */
                kernel_puts("sem novos eventos de lixo/leitura instavel\n");
            }
            NM_OUT_END();
        }
        mask = nm;

        if ((int32_t)(systemTick - t_print) >= 0) {       /* status periodico */
            NM_OUT_BEGIN();
            nm_print_status(!(nm & 2UL));
            NM_OUT_END();
            t_print = systemTick + NM_PRINT_S * 100UL;
        }
    }
}

static void nm_start(void)
{
    sys_thread_new("netmon", nm_thread, NULL, NM_STACK, 1);
}

#endif /* W5100_NETMON_H */
