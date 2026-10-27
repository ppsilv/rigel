/*
 * settime.c -- acerta o RTC do Rigel pela rede (protocolo TIME, RFC 868, TCP porta 37).
 *
 *   settime [-n] [-z HORAS] [-p PORTA] IP
 *
 *     IP       servidor de hora (nao ha DNS no Rigel). Ex.: 129.6.15.28 (time-a-g.nist.gov)
 *     -n       so' mostra; nao grava no relogio
 *     -z H     soma H horas a hora do servidor antes de gravar (ex.: -z -3 para hora do Brasil).
 *              Sem -z grava UTC, que e' o que o servidor manda (sys_readrtc e' segundos desde 1970).
 *     -p P     porta do servidor (padrao 37)
 *
 * Protocolo TIME: o servidor aceita a conexao, manda 4 bytes (big-endian) com os segundos
 * desde 01/01/1900 e fecha. Precisao de 1 segundo.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <rigel/rigel.h>
#include <rigel/netdefs.h>

#define TIME_PORT            37
#define TIMEOUT_MS           8000UL                 /* connect e read: desiste depois de 8 s */
#define SECS_1900_TO_1970    2208988800UL
#define MIN_VALID            1700000000UL           /* nov/2023: abaixo disso a resposta e' lixo */
#define MAX_VALID            4000000000UL

static uint16_t swap_if_le16(uint16_t v)            /* porta: no 68000 nao faz nada */
{
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return (uint16_t)((v >> 8) | (v << 8));
#else
    return v;
#endif
}

static uint32_t be32(const unsigned char *p)
{
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | p[3];
}

/* le exatamente n bytes; devolve quantos conseguiu */
static long read_full(int fd, unsigned char *p, long n)
{
    long got = 0;
    while (got < n) {
        long r = read(fd, p + got, (size_t)(n - got));
        if (r <= 0) break;
        got += r;
    }
    return got;
}

/* segundos desde 1970 -> data e hora (algoritmo "civil from days") */
static void unix_to_date(uint32_t t, int *y, int *mo, int *d, int *h, int *mi, int *s)
{
    uint32_t days = t / 86400UL, rem = t % 86400UL;
    long z   = (long)days + 719468L;
    long era = z / 146097L;
    long doe = z - era * 146097L;
    long yoe = (doe - doe / 1460L + doe / 36524L - doe / 146096L) / 365L;
    long yy  = yoe + era * 400L;
    long doy = doe - (365L * yoe + yoe / 4L - yoe / 100L);
    long mp  = (5L * doy + 2L) / 153L;
    long dd  = doy - (153L * mp + 2L) / 5L + 1L;
    long mm  = (mp < 10) ? mp + 3 : mp - 9;

    if (mm <= 2) yy++;
    *y = (int)yy; *mo = (int)mm; *d = (int)dd;
    *h = (int)(rem / 3600UL); *mi = (int)((rem % 3600UL) / 60UL); *s = (int)(rem % 60UL);
}

static void show(const char *label, uint32_t t)
{
    int y, mo, d, h, mi, s;
    unix_to_date(t, &y, &mo, &d, &h, &mi, &s);
    printf("%-9s %04d-%02d-%02d %02d:%02d:%02d  (%lu)\n", label, y, mo, d, h, mi, s, (unsigned long)t);
}

static int parse_ip(const char *s, uint32_t *ip)
{
    unsigned a, b, c, d;
    char extra;
    if (sscanf(s, "%u.%u.%u.%u%c", &a, &b, &c, &d, &extra) != 4) return -1;
    if (a > 255 || b > 255 || c > 255 || d > 255) return -1;
    *ip = RIGEL_IP(a, b, c, d);
    return 0;
}

/* pergunta a hora ao servidor; devolve 0 e os segundos desde 1900, ou -1 */
static int get_server_time(uint32_t ip, uint16_t port, uint32_t *t1900)
{
    rigel_sockaddr_in a;
    unsigned char b[4];
    uint32_t tmo = (uint32_t)TIMEOUT_MS;
    long s, n;

    s = rigel_socket(RIGEL_AF_INET, RIGEL_SOCK_STREAM, 0);
    if (s < 0) { printf("socket falhou (a rede esta iniciada?)\n"); return -1; }

    rigel_setsockopt(s, RIGEL_SOL_SOCKET, RIGEL_SO_RCVTIMEO, &tmo, sizeof tmo);
    rigel_setsockopt(s, RIGEL_SOL_SOCKET, RIGEL_SO_SNDTIMEO, &tmo, sizeof tmo);

    memset(&a, 0, sizeof a);
    a.sin_family = RIGEL_AF_INET;
    a.sin_port   = swap_if_le16(port);
    a.sin_addr   = ip;
    if (rigel_connect(s, &a, sizeof a) < 0) {
        printf("nao consegui conectar no servidor (IP/porta errados, sem rota ou bloqueado)\n");
        close((int)s);
        return -1;
    }

    n = read_full((int)s, b, 4);
    close((int)s);
    if (n != 4) {
        printf("o servidor nao respondeu os 4 bytes (recebi %ld)\n", n);
        return -1;
    }
    *t1900 = be32(b);
    return 0;
}

int main(int argc, char *argv[])
{
    uint32_t ip = 0, t1900, server, target, before, after;
    unsigned long port = TIME_PORT;
    long zone = 0, diff;
    int dry = 0, have_ip = 0, i;

    for (i = 0; i < argc; i++)
        printf("argv[%d]='%s'\n", i, argv[i]);

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-n") == 0) {
            dry = 1;
        } else if (strcmp(argv[i], "-z") == 0 && i + 1 < argc) {
            zone = strtol(argv[++i], NULL, 10);
        } else if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
            port = strtoul(argv[++i], NULL, 10);
        } else if (parse_ip(argv[i], &ip) == 0) {
            have_ip = 1;
        } else {
            printf("argumento invalido: %s\n", argv[i]);
            have_ip = 0;
            break;
        }
    }
    if (!have_ip || port == 0 || port > 65535 || zone < -14 || zone > 14) {
        printf("uso: settime [-n] [-z HORAS] [-p PORTA] IP\n");
        printf("ex.: settime -n 129.6.15.28      (so' mostra)\n");
        printf("     settime 129.6.15.28         (grava UTC)\n");
        printf("     settime -z -3 129.6.15.28   (grava hora do Brasil)\n");
        return 1;
    }

    before = (uint32_t)rigel_readrtc();

    if (get_server_time(ip, (uint16_t)port, &t1900) != 0)
        return 1;

    server = t1900 - (uint32_t)SECS_1900_TO_1970;      /* a conta em 32 bits tambem vale depois de 2036 */
    if (server < MIN_VALID || server > MAX_VALID) {
        printf("resposta invalida do servidor (%lu); relogio nao alterado\n", (unsigned long)t1900);
        return 1;
    }
    target = server + (uint32_t)(zone * 3600L);
    diff   = (long)(int32_t)(target - before);

    show("servidor", server);
    show("rtc agora", before);
    show("novo", target);
    printf("diferenca: %ld s (%s)\n", diff, diff > 0 ? "o relogio estava atrasado" :
                                            diff < 0 ? "o relogio estava adiantado" : "igual");

    if (dry) {
        printf("-n: relogio nao alterado\n");
        return 0;
    }

    rigel_writertc((long)target);
    after = (uint32_t)rigel_readrtc();
    show("rtc depois", after);
    {
        long e = (long)(int32_t)(after - target);
        if (e < -2 || e > 2)
            printf("aviso: o relogio ficou %ld s longe do esperado (gravou mesmo?)\n", e);
        else
            printf("ok\n");
    }
    return 0;
}