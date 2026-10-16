#ifndef W5100_NETIF_H
#define W5100_NETIF_H
/* Inicia a rede: W5100 em MACRAW + lwIP. Chame UMA vez no boot do kernel (ver instrucoes). 0 = ok. */
int rigel_net_start(void);
/* IP atual (formato RIGEL_IP) ou 0 se ainda nao tem (DHCP) */
uint32_t rigel_net_ip(void);

#endif
