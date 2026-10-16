#ifndef RIGEL_NETCFG_H
#define RIGEL_NETCFG_H
/* Configuracao fixa do W5100 (nao ha DHCP nesta versao). AJUSTE para a sua rede. */
#define NET_MAC   { 0x00, 0x23, 0x69, 0x44, 0x52, 0x5C }
#define NET_IP    { 192, 168, 1, 50 }
#define NET_GW    { 192, 168, 1, 1 }
#define NET_MASK  { 255, 255, 255, 0 }
/* Para usar DHCP em vez do IP fixo: defina NET_USE_DHCP aqui e LWIP_DHCP 1 no lwipopts.h */
/* #define NET_USE_DHCP */
#endif
