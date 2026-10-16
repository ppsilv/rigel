#include "lwip/opt.h"
#include "lwip/def.h"
#include "lwip/pbuf.h"
#include "lwip/netif.h"
#include "lwip/etharp.h"
#include "lwip/tcpip.h"

/* Suas funções de hardware existentes */
extern void w5100_write(const void *buf, u16_t len);
extern void w5100_read(void *buf, u16_t len);
extern u16_t w5100_get_received_packet_len(void);

/* -----------------------------------------------------------------
 * 1. A SAÍDA (O lwIP chama isso quando quer enviar algo)
 * ----------------------------------------------------------------- */
static err_t rigel_w5100_linkoutput(struct netif *netif, struct pbuf *p)
{
    struct pbuf *q;
    
    // Percorre os pedaços do pacote e joga no hardware via w5100_write
    for (q = p; q != NULL; q = q->next) {
        w5100_write(q->payload, q->len);
    }

    return ERR_OK;
}

/* -----------------------------------------------------------------
 * 2. A ENTRADA / POLLING (Você chama isso na sua task ou interrupção)
 * ----------------------------------------------------------------- */
void rigel_w5100_poll_or_isr(void)
{
    u16_t len = w5100_get_received_packet_len();
    if (len == 0) return;

    // Aloca buffer no lwIP
    struct pbuf *p = pbuf_alloc(PBUF_RAW, len, PBUF_POOL);
    if (p != NULL) {
        // Lê do hardware para dentro do buffer do lwIP
        w5100_read(p->payload, len);

        // Como você está usando OS (NO_SYS=0), entrega para a thread do lwIP via tcpip_input
        if (netif.input(p, &netif) != ERR_OK) {
            pbuf_free(p);
        }
    }
}

/* -----------------------------------------------------------------
 * 3. A INICIALIZAÇÃO DO DRIVER (Chamada no boot do sistema)
 * ----------------------------------------------------------------- */
err_t rigel_w5100_init(struct netif *netif)
{
    netif->name[0] = 'e';
    netif->name[1] = '0';

    // Amarra o ponteiro de saída do lwIP à sua função de link
    netif->linkoutput = rigel_w5100_linkoutput;
    netif->output     = etharp_output;

    netif->mtu        = 1500;
    netif->flags      = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP;

    // TODO: Inicialize os registradores físicos do W5100 aqui se precisar

    return ERR_OK;
}
