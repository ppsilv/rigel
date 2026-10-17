/*
 * lwipopts.h -- configuracao do lwIP para o Rigel.
 * Se voce ja tem um lwipopts.h, confira se as opcoes abaixo estao iguais.
 */
#ifndef LWIPOPTS_H
#define LWIPOPTS_H

/* ---- sistema ---- */
#define NO_SYS                      0
#define SYS_LIGHTWEIGHT_PROT        1
#define LWIP_TCPIP_CORE_LOCKING     0
#define MEM_ALIGNMENT               2          /* 68000: acesso de 16/32 bits precisa de endereco par */
/* Memoria do lwIP.
 * RIGEL_LWIP_OWN_HEAP 1: o lwIP usa o heap dele (mem.c), de MEM_SIZE bytes, reservado UMA vez do kmalloc
 *                        na partida; o kmalloc nao mexe nos pbufs/pcbs. (teste para isolar problema no kmalloc)
 * RIGEL_LWIP_OWN_HEAP 0: cada alocacao do lwIP vai para o kmalloc (versao anterior). */
#include <stddef.h>
#define RIGEL_LWIP_OWN_HEAP         1
#if RIGEL_LWIP_OWN_HEAP
#define MEM_LIBC_MALLOC             0
#define MEMP_MEM_MALLOC             1
#define MEM_SIZE                    (64 * 1024)
void *rigel_lwip_heap(void);
#define LWIP_RAM_HEAP_POINTER       rigel_lwip_heap()
#define LWIP_ALLOW_MEM_FREE_FROM_OTHER_CONTEXT 1   /* protege o heap mascarando IRQ (sem mutex) */
#else
void *rigel_malloc(size_t n);
void  rigel_free(void *p);
void *rigel_calloc(size_t count, size_t size);
#define MEM_LIBC_MALLOC             1
#define MEMP_MEM_MALLOC             1
#define mem_clib_malloc             rigel_malloc
#define mem_clib_free               rigel_free
#define mem_clib_calloc             rigel_calloc
#endif

/* ---- API ---- */
#define LWIP_NETCONN                1
#define LWIP_SOCKET                 1
#define LWIP_COMPAT_SOCKETS         0          /* so' lwip_socket(), lwip_bind()... (nao colide com o kernel) */
#define LWIP_POSIX_SOCKETS_IO_NAMES 0
#define LWIP_SOCKET_SELECT          0
#define LWIP_SOCKET_POLL            0
#define LWIP_SO_RCVTIMEO            1
#define LWIP_SO_SNDTIMEO            1
#define LWIP_SO_SNDRCVTIMEO_NONSTANDARD 1      /* timeout = int em milissegundos */
#define LWIP_TIMEVAL_PRIVATE        0          /* newlib ja tem struct timeval */
#define MEMP_NUM_NETCONN            8
#define LWIP_SOCKET_OFFSET          0

/* ---- protocolos ---- */
#define LWIP_IPV4                   1
#define LWIP_IPV6                   0
#define LWIP_ARP                    1
#define LWIP_ICMP                   1
#define LWIP_RAW                    1
#define LWIP_UDP                    1
#define LWIP_TCP                    1
#define LWIP_DHCP                   0          /* 1 + NET_USE_DHCP no rigel_netcfg.h para usar DHCP */
#define LWIP_DNS                    0
#define LWIP_IGMP                   0
#define LWIP_STATS                  0

/* ---- memoria / TCP ---- */
#define TCP_MSS                     1460
#define TCP_WND                     (2 * TCP_MSS)
#define TCP_SND_BUF                 (2 * TCP_MSS)
#define MEMP_NUM_TCP_PCB            8
#define MEMP_NUM_TCP_PCB_LISTEN     4
#define MEMP_NUM_TCP_SEG            16
#define PBUF_POOL_SIZE              12
#define PBUF_POOL_BUFSIZE           1520
#define MEMP_NUM_PBUF               8
#define MEMP_NUM_SYS_TIMEOUT        12

/* ---- threads ---- */
#define TCPIP_THREAD_NAME           "tcpip"
#define TCPIP_THREAD_STACKSIZE      8192
#define TCPIP_THREAD_PRIO           1
#define TCPIP_MBOX_SIZE             16
#define DEFAULT_TCP_RECVMBOX_SIZE   8
#define DEFAULT_UDP_RECVMBOX_SIZE   8
#define DEFAULT_RAW_RECVMBOX_SIZE   8
#define DEFAULT_ACCEPTMBOX_SIZE     8

#define LWIP_NETIF_STATUS_CALLBACK  0
#define LWIP_NETIF_LINK_CALLBACK    0
#define ETH_PAD_SIZE                0
#define LWIP_CHKSUM_ALGORITHM       2

#endif
