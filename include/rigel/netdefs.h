#ifndef RIGEL_NETDEFS_H
#define RIGEL_NETDEFS_H
/*
 * rigel_net.h -- constantes e estruturas de rede do Rigel (kernel e apps).
 * Master em  kernel/include/sys/rigel_net.h ; copia em  librigel/include/rigel/netdefs.h
 *
 * O 68000 e' big-endian, igual a rede: NAO precisa de htons/htonl.
 *   porta:  numero normal              (ex.: 7000)
 *   IP:     RIGEL_IP(192,168,1,10)
 */
#include <stdint.h>

#define RIGEL_AF_INET        2
#define RIGEL_SOCK_STREAM    1          /* so' TCP nesta versao */
#define RIGEL_IPPROTO_TCP    6
#define RIGEL_INADDR_ANY     0UL

#define RIGEL_IP(a,b,c,d)  ((((uint32_t)(a)) << 24) | (((uint32_t)(b)) << 16) | \
                            (((uint32_t)(c)) <<  8) |  ((uint32_t)(d)))

typedef struct {
    uint16_t sin_family;                /* RIGEL_AF_INET */
    uint16_t sin_port;                  /* porta */
    uint32_t sin_addr;                  /* IP (RIGEL_IP) */
    uint8_t  sin_zero[8];
} rigel_sockaddr_in;                    /* 16 bytes, como o sockaddr_in do BSD */

/* setsockopt */
#define RIGEL_SOL_SOCKET     1
#define RIGEL_SO_REUSEADDR   2          /* aceito e ignorado */
#define RIGEL_SO_KEEPALIVE   9          /* aceito e ignorado (o W5100 nao tem) */
#define RIGEL_SO_RCVTIMEO    20         /* optval = uint32_t *, milissegundos; 0 = sem limite (read/accept) */
#define RIGEL_SO_SNDTIMEO    21         /* optval = uint32_t *, milissegundos; 0 = padrao (write 10 s, connect 30 s) */
#define RIGEL_IPPROTO_TCP_LV 6
#define RIGEL_TCP_NODELAY    1          /* aceito e ignorado */

#endif
