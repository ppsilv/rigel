/*
 * w5100_netif.c -- driver lwIP (netif ethernet) para o WIZnet W5100 em modo MACRAW.
 *
 * O W5100 so' entrega quadros Ethernet crus no socket 0 (MACRAW); o lwIP faz ARP/IP/TCP.
 * Todo o buffer do chip vai para o socket 0: RMSR = TMSR = 0x03 (8 KB de RX e 8 KB de TX).
 *
 * Registradores: BASE + off*2 + 1 (so' os bytes impares do barramento).
 * Recepcao: uma task (w5100rx) olha o chip a cada tick (10 ms) e entrega os quadros ao lwIP.
 * Cada quadro no buffer de RX vem precedido de 2 bytes com (tamanho do quadro + 2).
 */
#include <stdint.h>
#include <string.h>

#include "lwip/opt.h"
#include "lwip/def.h"
#include "lwip/mem.h"
#include "lwip/pbuf.h"
#include "lwip/netif.h"
#include "lwip/etharp.h"
#include "lwip/tcpip.h"
#include "lwip/dhcp.h"
#include "lwip/sys.h"
#include "netif/ethernet.h"

#include "w5100_netif.h"
#include "rigel_netcfg.h"

/* ---------------------------------------------------------------- acesso ao chip */
#ifdef NET_HOST_TEST
extern uint8_t net_hw_rd(uint16_t reg);
extern void    net_hw_wr(uint16_t reg, uint8_t v);
#define HW_RD(r)      net_hw_rd(r)
#define HW_WR(r, v)   net_hw_wr((r), (v))
#else
#define W5100_BASE    0xFE0000UL
#define W5100_REG(o)  ((volatile uint8_t *)(W5100_BASE + ((unsigned long)(o) << 1) + 1UL))
#define HW_RD(r)      (*W5100_REG(r))
#define HW_WR(r, v)   (*W5100_REG(r) = (uint8_t)(v))
#endif

extern volatile uint32_t systemTick;           /* +1 a cada 10 ms */
extern void kernel_puts(const char *s);
extern void rigel_stack_report(int all);       /* sys_arch.c */

/* ---- depuracao: ponha 1 para ver na tela cada passo (so' ARP e quadros para o nosso MAC) ---- */
#define NET_DEBUG 0
#if NET_DEBUG
static void dbg_num(uint32_t v)                /* decimal */
{
    char b[12]; int i = 11; b[i] = 0;
    if (v == 0) { kernel_puts("0"); return; }
    while (v && i > 0) { b[--i] = (char)('0' + v % 10); v /= 10; }
    kernel_puts(&b[i]);
}
static void dbg_hex16(uint16_t v)
{
    static const char h[] = "0123456789ABCDEF";
    char b[5]; b[0] = h[(v >> 12) & 15]; b[1] = h[(v >> 8) & 15]; b[2] = h[(v >> 4) & 15]; b[3] = h[v & 15]; b[4] = 0;
    kernel_puts(b);
}
#define DBG(s)  kernel_puts(s)
#else
#define DBG(s)  do { } while (0)
#endif

#define W_MR      0x0000
#define W_SHAR    0x0009
#define W_RMSR    0x001A
#define W_TMSR    0x001B

#define S0_MR     0x0400
#define S0_CR     0x0401
#define S0_IR     0x0402
#define S0_SR     0x0403
#define S0_TX_FSR 0x0420
#define S0_TX_WR  0x0424
#define S0_RX_RSR 0x0426
#define S0_RX_RD  0x0428

#define MR_RESET     0x80
#define S_MR_MACRAW  0x04
#define S_MR_MF      0x40                      /* filtro de MAC: so' o meu, broadcast e multicast */
#define CMD_OPEN     0x01
#define CMD_CLOSE    0x10
#define CMD_SEND     0x20
#define CMD_RECV     0x40
#define IR_SEND_OK   0x10
#define SOCK_MACRAW  0x42

#define TX_BASE      0x4000
#define RX_BASE      0x6000
#define BUF_MASK     0x1FFF                    /* 8 KB */
#define MAX_FRAME    1518

static inline uint16_t rd16(uint16_t r) { return (uint16_t)((HW_RD(r) << 8) | HW_RD(r + 1)); }
static inline void     wr16(uint16_t r, uint16_t v) { HW_WR(r, v >> 8); HW_WR(r + 1, v & 0xFF); }

static uint16_t stable16(uint16_t r)           /* RSR/FSR: leitura dupla ate estabilizar */
{
    uint16_t a, b;
    do { a = rd16(r); b = rd16(r); } while (a != b);
    return a;
}

static int cmd0(uint8_t c)
{
    uint32_t t0 = systemTick;
    HW_WR(S0_CR, c);
    while (HW_RD(S0_CR)) {
        if ((uint32_t)(systemTick - t0) > 50) return -1;
    }
    return 0;
}

/* ---------------------------------------------------------------- estado */
static struct netif w5100_if;
static sys_mutex_t  hw_lock;
static int          started;
static const uint8_t cfg_mac[6] = NET_MAC;

/* gerador pseudo-aleatorio (ISN do TCP, portas efemeras) */
uint32_t rigel_rand32(void)
{
    static uint32_t s;
    if (s == 0) s = 0x1234ABCDUL ^ (systemTick * 2654435761UL);
    s ^= s << 13;
    s ^= s >> 17;
    s ^= s << 5;
    return s;
}

/* ---------------------------------------------------------------- chip */
static int hw_init(void)
{
    uint32_t t0;
    int i;

    HW_WR(W_MR, MR_RESET);
    t0 = systemTick;
    while (HW_RD(W_MR) & MR_RESET) {
        if ((uint32_t)(systemTick - t0) > 50) return -1;
    }
    HW_WR(W_MR, 0);
    for (i = 0; i < 6; i++) HW_WR(W_SHAR + i, cfg_mac[i]);
    HW_WR(W_RMSR, 0x03);                       /* socket 0 com 8 KB, os outros 0 */
    HW_WR(W_TMSR, 0x03);

    HW_WR(S0_MR, S_MR_MACRAW | S_MR_MF);
    if (cmd0(CMD_OPEN) != 0) return -1;
    if (HW_RD(S0_SR) != SOCK_MACRAW) return -1;
    return 0;
}

/* ---------------------------------------------------------------- envio */
static err_t low_level_output(struct netif *netif, struct pbuf *p)
{
    struct pbuf *q;
    uint16_t len = p->tot_len, ptr;
    uint32_t t0;
    uint16_t i;
    (void)netif;

    if (len == 0 || len > MAX_FRAME) return ERR_BUF;

#if NET_DEBUG
    kernel_puts("  TX len="); dbg_num(len); kernel_puts("\n");
#endif
    sys_mutex_lock(&hw_lock);
    DBG("  TX lock ok\n");

    t0 = systemTick;
    while (stable16(S0_TX_FSR) < len) {        /* espera espaco */
        if ((uint32_t)(systemTick - t0) > 100) { sys_mutex_unlock(&hw_lock); return ERR_TIMEOUT; }
    }

    ptr = rd16(S0_TX_WR);
    for (q = p; q != NULL; q = q->next) {
        const uint8_t *s = (const uint8_t *)q->payload;
        for (i = 0; i < q->len; i++)
            HW_WR(TX_BASE + ((ptr++) & BUF_MASK), s[i]);
    }
    wr16(S0_TX_WR, ptr);
    DBG("  TX dados no chip\n");

    HW_WR(S0_IR, IR_SEND_OK);
    if (cmd0(CMD_SEND) != 0) { sys_mutex_unlock(&hw_lock); return ERR_IF; }

    t0 = systemTick;
    while (!(HW_RD(S0_IR) & IR_SEND_OK)) {
        if ((uint32_t)(systemTick - t0) > 100) break;
    }
    HW_WR(S0_IR, IR_SEND_OK);
    DBG("  TX enviado\n");

    sys_mutex_unlock(&hw_lock);
    return ERR_OK;
}

/* ---------------------------------------------------------------- recepcao */
/* devolve um pbuf com um quadro ou NULL se nao ha nenhum */
static struct pbuf *low_level_input(void)
{
    struct pbuf *p = NULL, *q;
    uint16_t rsr, ptr, len, i;

    sys_mutex_lock(&hw_lock);

    rsr = stable16(S0_RX_RSR);
    if (rsr < 2) goto out;

    ptr = rd16(S0_RX_RD);
    len = (uint16_t)((HW_RD(RX_BASE + (ptr & BUF_MASK)) << 8) | HW_RD(RX_BASE + ((ptr + 1) & BUF_MASK)));
    if (len < 2 || len - 2 > MAX_FRAME || len > rsr) {      /* lixo: descarta tudo o que esta no buffer */
        wr16(S0_RX_RD, (uint16_t)(ptr + rsr));
        cmd0(CMD_RECV);
        goto out;
    }
    len -= 2;
    ptr += 2;

    p = pbuf_alloc(PBUF_RAW, len, PBUF_POOL);
    for (q = p; q != NULL; q = q->next) {
        uint8_t *d = (uint8_t *)q->payload;
        for (i = 0; i < q->len; i++)
            d[i] = HW_RD(RX_BASE + ((ptr++) & BUF_MASK));
    }
    if (p == NULL)
        ptr += len;                                         /* sem memoria: pula o quadro */
    wr16(S0_RX_RD, ptr);
    cmd0(CMD_RECV);
out:
    sys_mutex_unlock(&hw_lock);
    return p;
}

static void rx_thread(void *arg)
{
    struct netif *nif = (struct netif *)arg;
    struct pbuf *p;
    uint32_t t, n = 0;

    for (;;) {
        /* a cada ~30 s avisa se alguma pilha de thread esta quase cheia; aos ~60 s mostra todas */
        n++;
        //if (n % 3000 == 0)  rigel_stack_report(0);         /* a cada ~30 s: so' se alguma pilha estiver baixa */
        //if (n == 6000)      rigel_stack_report(1);         /* uma unica vez, aos ~60 s: mostra todas */
        if (n >= 360000)    n = 6001;                      /* evita estouro do contador; nao repete o relatorio */

        while ((p = low_level_input()) != NULL) {
#if NET_DEBUG
            {
                const uint8_t *f = (const uint8_t *)p->payload;
                uint16_t et = (uint16_t)((f[12] << 8) | f[13]);
                if (memcmp(f, cfg_mac, 6) == 0 || et == 0x0806) {
                    kernel_puts("RX len="); dbg_num(p->tot_len);
                    kernel_puts(" tipo="); dbg_hex16(et); kernel_puts("\n");
                }
            }
#endif
            if (nif->input(p, nif) != ERR_OK)
                pbuf_free(p);
            DBG("  RX entregue ao lwIP\n");
        }
        t = systemTick;                                     /* espera o proximo tick */
        while (systemTick == t)
            ;
    }
}

/* ---------------------------------------------------------------- netif */
static err_t w5100_netif_init(struct netif *netif)
{
    DBG("  netif init callback (inicio)\n");
    netif->name[0] = 'w';
    netif->name[1] = '5';
    netif->output     = etharp_output;
    netif->linkoutput = low_level_output;
    netif->hwaddr_len = 6;
    memcpy(netif->hwaddr, cfg_mac, 6);
    netif->mtu   = 1500;
    netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP;
    DBG("  netif init callback (fim)\n");
    return ERR_OK;
}

uint32_t rigel_net_ip(void)
{
    const ip4_addr_t *a;
    if (!started) return 0;
    a = netif_ip4_addr(&w5100_if);
    return ip4_addr_get_u32(a);          /* 68000 e rede sao big-endian: ja esta no formato RIGEL_IP */
}

int rigel_net_start(void)
{
    ip4_addr_t ip, mask, gw;
    static const uint8_t cip[4] = NET_IP, cgw[4] = NET_GW, cmask[4] = NET_MASK;

    kernel_puts("    :w5100_netif.c: rigel_net_start\n");
    if (started) return 0;
    if (sys_mutex_new(&hw_lock) != ERR_OK) return -1;
    if (hw_init() != 0) return -1;

    kernel_puts("    :tcpip initializing\n");
    tcpip_init(NULL, NULL);

#ifdef NET_USE_DHCP
    kernel_puts("    :dhcp calling\n");
    ip4_addr_set_zero(&ip); ip4_addr_set_zero(&mask); ip4_addr_set_zero(&gw);
#else
    kernel_puts("    :static ip setting\n");
    IP4_ADDR(&ip,   cip[0], cip[1], cip[2], cip[3]);
    IP4_ADDR(&gw,   cgw[0], cgw[1], cgw[2], cgw[3]);
    IP4_ADDR(&mask, cmask[0], cmask[1], cmask[2], cmask[3]);
#endif
    kernel_puts("    :adding an ethernet interface\n");
    if (netif_add(&w5100_if, &ip, &mask, &gw, NULL, w5100_netif_init, tcpip_input) == NULL)
        return -1;
    DBG("  netif_add retornou\n");
    kernel_puts("    :setting default ethernet interface\n");
    netif_set_default(&w5100_if);
    kernel_puts("    :setup of ethernet interface\n");
    netif_set_up(&w5100_if);
    kernel_puts("    :setting ethernet interface up\n");
    netif_set_link_up(&w5100_if);
#ifdef NET_USE_DHCP
    kernel_puts("    :dhcp start\n");
    dhcp_start(&w5100_if);
#endif

    kernel_puts("    :w5100rx thread creating\n");
    if (sys_thread_new("w5100rx", rx_thread, &w5100_if, 8192, 1) < 0)
        return -1;
    started = 1;
    return 0;
}
