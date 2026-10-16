/*
 * nettest.c -- testa os syscalls de rede do Rigel (socket/bind/listen/accept/connect/setsockopt).
 *
 *   nettest server PORTA                 escuta; ecoa tudo que o cliente mandar; "quit" encerra
 *   nettest client IP PORTA TEXTO        conecta, manda TEXTO, imprime a resposta
 *   nettest http IP PORTA                manda "GET /" e imprime o comeco da resposta
 *
 * Exemplos (do PC, que tem IP 192.168.1.10):
 *   no Rigel:  nettest server 7000      no PC:  nc 192.168.1.173 7000
 *   no PC:     nc -l 7000               no Rigel: nettest client 192.168.1.10 7000 ola
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <rigel/rigel.h>
#include <rigel/netdefs.h>

/* "a.b.c.d" -> RIGEL_IP; devolve 0 se invalido */
static int parse_ip(const char *s, uint32_t *out)
{
    uint32_t ip = 0;
    int i;
    for (i = 0; i < 4; i++) {
        char *end;
        unsigned long v = strtoul(s, &end, 10);
        if (end == s || v > 255) return 0;
        ip = (ip << 8) | (uint32_t)v;
        if (i < 3) { if (*end != '.') return 0; s = end + 1; }
        else if (*end != '\0') return 0;
    }
    *out = ip;
    return 1;
}

static void fill(rigel_sockaddr_in *a, uint32_t ip, unsigned port)
{
    memset(a, 0, sizeof *a);
    a->sin_family = RIGEL_AF_INET;
    a->sin_port   = (uint16_t)port;
    a->sin_addr   = ip;
}

static void ip_str(uint32_t ip, char *b)
{
    sprintf(b, "%u.%u.%u.%u", (unsigned)(ip >> 24), (unsigned)((ip >> 16) & 255),
            (unsigned)((ip >> 8) & 255), (unsigned)(ip & 255));
}

static int do_server(unsigned port)
{
    rigel_sockaddr_in a, peer;
    uint32_t one = 1;
    long plen, ls, cs;
    char buf[128], ipb[20];
    int quit = 0;

    ls = rigel_socket(RIGEL_AF_INET, RIGEL_SOCK_STREAM, 0);
    if (ls < 0) { printf("socket falhou (a rede esta iniciada?)\n"); return 1; }
    rigel_setsockopt(ls, RIGEL_SOL_SOCKET, RIGEL_SO_REUSEADDR, &one, sizeof one);
    fill(&a, RIGEL_INADDR_ANY, port);
    if (rigel_bind(ls, &a, sizeof a) < 0)   { printf("bind falhou\n");   return 1; }
    if (rigel_listen(ls, 2) < 0)            { printf("listen falhou\n"); return 1; }
    printf("escutando na porta %u...\n", port);

    while (!quit) {
        int n;
        plen = sizeof peer;
        cs = rigel_accept(ls, &peer, &plen);
        if (cs < 0) { printf("accept falhou\n"); break; }
        ip_str(peer.sin_addr, ipb);
        printf("cliente %s:%u\n", ipb, (unsigned)peer.sin_port);
        while ((n = read((int)cs, buf, sizeof buf - 1)) > 0) {
            write((int)cs, buf, n);                 /* eco */
            buf[n] = 0;
            printf("recebi %d bytes: %s", n, buf);
            if (strncmp(buf, "quit", 4) == 0) { quit = 1; break; }
        }
        close((int)cs);
        printf("\ncliente saiu\n");
    }
    close((int)ls);
    return 0;
}

static int do_client(const char *ips, unsigned port, const char *text, int http)
{
    rigel_sockaddr_in a;
    uint32_t ip, tmo = 5000;
    long s;
    char buf[512];
    int n;

    if (!parse_ip(ips, &ip)) { printf("IP invalido: %s\n", ips); return 1; }
    s = rigel_socket(RIGEL_AF_INET, RIGEL_SOCK_STREAM, 0);
    if (s < 0) { printf("socket falhou (a rede esta iniciada?)\n"); return 1; }
    rigel_setsockopt(s, RIGEL_SOL_SOCKET, RIGEL_SO_RCVTIMEO, &tmo, sizeof tmo);
    fill(&a, ip, port);
    if (rigel_connect(s, &a, sizeof a) < 0) { printf("connect falhou\n"); close((int)s); return 1; }
    printf("conectado a %s:%u\n", ips, port);

    if (http) text = "GET / HTTP/1.0\r\nHost: rigel\r\n\r\n";
    write((int)s, text, strlen(text));

    while ((n = read((int)s, buf, sizeof buf - 1)) > 0) {
        buf[n] = 0;
        fputs(buf, stdout);
        if (!http) break;                            /* eco: uma resposta basta */
    }
    printf("\n(fim: read = %d)\n", n);
    close((int)s);
    return 0;
}

int main(int argc, char **argv)
{
    if (argc >= 3 && strcmp(argv[1], "server") == 0)
        return do_server((unsigned)atoi(argv[2]));
    if (argc >= 5 && strcmp(argv[1], "client") == 0)
        return do_client(argv[2], (unsigned)atoi(argv[3]), argv[4], 0);
    if (argc >= 4 && strcmp(argv[1], "http") == 0)
        return do_client(argv[2], (unsigned)atoi(argv[3]), "", 1);
    printf("uso: nettest server PORTA | client IP PORTA TEXTO | http IP PORTA\n");
    return 1;
}
