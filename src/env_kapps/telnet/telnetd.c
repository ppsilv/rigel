/*
 * telnetd.c -- servidor telnet do Rigel (SEM autenticacao: so' para rede local).
 *
 *   telnetd [porta [programa]]       padrao: porta 23, programa /bin/shell.elf
 *
 * Uma sessao por vez. Para cada conexao:
 *   1) cria um pty (rigel_openpty);
 *   2) inicia o shell com a ponta slave como stdin/stdout/stderr (rigel_spawn_io);
 *   3) fica copiando bytes: socket -> pty (teclas) e pty -> socket (tela).
 * A sessao acaba quando o cliente desconecta ou o shell termina.
 *
 * Premissas sobre o socket (ver net_read): o read espera no maximo POLL_MS
 * (SO_RCVTIMEO) e devolve  n>0 dados | 0 conexao fechada | n<0 estourou o tempo.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <rigel/rigel.h>
#include <rigel/netdefs.h>
#include "rigel_pty.h"
#include "telnet_proto.h"

#define DEFAULT_PORT  23
#define DEFAULT_PROG  "/bin/shell.elf"
#define POLL_MS       10            /* tempo maximo parado esperando o cliente */

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

/* manda tudo; -1 se a conexao caiu */
static int net_send(long cs, const uint8_t *p, size_t n)
{
    while (n > 0) {
        int w = write((int)cs, p, n);
        if (w <= 0)
            return -1;
        p += w;
        n -= (size_t)w;
    }
    return 0;
}

/* uma sessao telnet ate' o fim */
static void session(long cs, const char *prog)
{
    uint8_t rx[128], keys[128], tx[256], enc[512];
    size_t keys_len = 0, keys_off = 0;
    int pty[2];
    long pid;
    uint32_t tmo = POLL_MS;
    char *argv[2];
    tn_in_t in;

    tn_in_init(&in);
    rigel_setsockopt(cs, RIGEL_SOL_SOCKET, RIGEL_SO_RCVTIMEO, &tmo, sizeof tmo);

    /* poe o cliente em modo caractere antes de qualquer saida do shell */
    if (net_send(cs, tn_greeting, sizeof tn_greeting) < 0)
        return;

    if (rigel_openpty(pty) < 0) {
        static const char m[] = "telnetd: sem pty\r\n";
        net_send(cs, (const uint8_t *)m, sizeof m - 1);
        return;
    }
    argv[0] = (char *)prog;
    argv[1] = NULL;
    pid = rigel_spawn_io(prog, argv, pty[1], pty[1], pty[1]);
    close(pty[1]);          /* so' o shell segura a ponta dele: quando ele sair, a master ve o fim */
    if (pid < 0) {
        static const char m[] = "telnetd: nao consegui iniciar o shell\r\n";
        net_send(cs, (const uint8_t *)m, sizeof m - 1);
        close(pty[0]);
        return;
    }

    for (;;) {
        int n;

        /* 1) rede -> shell. O read do socket e' a espera de ate' POLL_MS da volta.
         * So' le o que cabe na fila de teclas pendentes (filtrar nunca gera mais bytes que leu). */
        if (keys_off == keys_len)
            keys_off = keys_len = 0;
        n = -1;
        if (keys_len < sizeof keys) {
            size_t room = sizeof keys - keys_len;
            n = read((int)cs, rx, room < sizeof rx ? room : sizeof rx);
        }
        if (n == 0)
            break;                                  /* o cliente fechou */
        if (n > 0)
            keys_len += tn_filter_in(&in, rx, (size_t)n, keys + keys_len);
        while (keys_off < keys_len) {               /* pty cheio: o resto vai na proxima volta */
            int w = write(pty[0], keys + keys_off, keys_len - keys_off);
            if (w < 0)
                goto fim;                           /* o shell ja' saiu */
            if (w == 0)
                break;
            keys_off += (size_t)w;
        }

        /* 2) shell -> rede */
        for (;;) {
            size_t m;
            n = read(pty[0], tx, sizeof tx);
            if (n < 0)
                goto fim;                           /* o shell terminou e a tela acabou */
            if (n == 0)
                break;
            m = tn_encode_out(tx, (size_t)n, enc);
            if (net_send(cs, enc, m) < 0)
                goto fim;
        }
    }

fim:
    /* cliente foi embora com o shell ainda vivo: pede para ele sair (nao ha syscall de kill) */
    write(pty[0], "\nexit\n", 6);
    close(pty[0]);
}

int main(int argc, char **argv)
{
    unsigned port = (argc > 1) ? (unsigned)atoi(argv[1]) : DEFAULT_PORT;
    const char *prog = (argc > 2) ? argv[2] : DEFAULT_PROG;
    rigel_sockaddr_in a, peer;
    uint32_t one = 1;
    long plen, ls, cs;
    char ipb[20];

    ls = rigel_socket(RIGEL_AF_INET, RIGEL_SOCK_STREAM, 0);
    if (ls < 0) { printf("telnetd: socket falhou (a rede esta iniciada?)\n"); return 1; }
    rigel_setsockopt(ls, RIGEL_SOL_SOCKET, RIGEL_SO_REUSEADDR, &one, sizeof one);
    fill(&a, RIGEL_INADDR_ANY, port);
    if (rigel_bind(ls, &a, sizeof a) < 0)  { printf("telnetd: bind falhou\n");   return 1; }
    if (rigel_listen(ls, 1) < 0)           { printf("telnetd: listen falhou\n"); return 1; }
    printf("telnetd: escutando na porta %u (%s)\n", port, prog);

    for (;;) {
        plen = sizeof peer;
        cs = rigel_accept(ls, &peer, &plen);
        if (cs < 0) { printf("telnetd: accept falhou\n"); break; }
        ip_str(peer.sin_addr, ipb);
        printf("telnetd: conexao de %s\n", ipb);
        session(cs, prog);
        close((int)cs);
        printf("telnetd: sessao encerrada\n");
    }
    close((int)ls);
    return 0;
}
