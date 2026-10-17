/*
 * rxfile.c -- recebe arquivos pela rede (TCP) e grava no disco.
 *
 *   rxfile [-l] [-1] PORTA [PASTA]
 *
 *     PORTA   porta TCP onde espera (ex.: 7001)
 *     PASTA   onde gravar (ex.: /dados). Sem PASTA grava na pasta atual, como o wfile.
 *     -l      os campos de 32 bits chegam em little-endian (PC x86 que manda a struct crua).
 *             Sem -l: big-endian ("ordem de rede", a mesma do 68000).
 *     -1      atende UM arquivo e termina (sem -1 fica esperando o proximo).
 *
 * Formato do que o PC envia (21 bytes de cabecalho, sem preenchimento, depois os dados):
 *
 *     bytes  0..3    tamanho dos dados (uint32)
 *     bytes  4..7    crc32 (uint32)   0 = nao verificar   (se vier diferente de 0, confere CRC-32 padrao)
 *     bytes  8..20   nome do arquivo, 13 bytes (8.3 + zero final), ex.: "TESTE.TXT"
 *     bytes 21..     dados: exatamente "tamanho" bytes
 *
 * (A struct FILE_RECV do PC tem o ponteiro "dados" no fim: ele NAO vai pela rede; os dados
 *  seguem logo depois do nome. O cabecalho e' montado byte a byte aqui, entao nao importa o
 *  alinhamento do compilador.)
 *
 * Se algo falha no meio (conexao cai, disco cheio, CRC errado) o arquivo parcial e' apagado.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <rigel/rigel.h>
#include <rigel/netdefs.h>

#define HDR_SIZE        21
#define NAME_LEN        13
#define BUF_SIZE        4096
#define RCV_TIMEOUT_MS  15000UL          /* sem receber nada por 15 s = desiste da conexao */
#define MAX_SIZE        0x10000000UL     /* 256 MB: acima disso o cabecalho esta errado */

static unsigned char buf[BUF_SIZE];
static uint32_t crc_tab[16];

/* ---------------------------------------------------------------- utilidades */
static uint16_t swap_if_le16(uint16_t v)  /* porta: ordem de rede <-> maquina (no 68000 nao faz nada) */
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

static uint32_t le32(const unsigned char *p)
{
    return ((uint32_t)p[3] << 24) | ((uint32_t)p[2] << 16) | ((uint32_t)p[1] << 8) | p[0];
}

static void crc_init(void)
{
    int i, k;
    for (i = 0; i < 16; i++) {
        uint32_t c = (uint32_t)i;
        for (k = 0; k < 4; k++)
            c = (c & 1) ? (c >> 1) ^ 0xEDB88320UL : c >> 1;
        crc_tab[i] = c;
    }
}

static uint32_t crc_update(uint32_t c, const unsigned char *p, long n)
{
    while (n-- > 0) {
        c ^= *p++;
        c = (c >> 4) ^ crc_tab[c & 15];
        c = (c >> 4) ^ crc_tab[c & 15];
    }
    return c;
}

static void ip_str(uint32_t ip, char *b)
{
    sprintf(b, "%u.%u.%u.%u", (unsigned)(ip >> 24), (unsigned)((ip >> 16) & 255),
            (unsigned)((ip >> 8) & 255), (unsigned)(ip & 255));
}

/* le exatamente n bytes; devolve quantos conseguiu (menos que n = conexao fechou ou deu erro) */
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

/* nome 8.3 vindo da rede: sem '/', '\\', ':' etc., nao vazio, nao "." nem ".." */
static int name_ok(const char *s)
{
    size_t i, n = strlen(s);
    if (n == 0) return 0;
    if (strcmp(s, ".") == 0 || strcmp(s, "..") == 0) return 0;
    for (i = 0; i < n; i++) {
        unsigned char c = (unsigned char)s[i];
        if (c < 0x20 || c >= 0x7F || strchr("/\\:*?\"<>|", c)) return 0;
    }
    return 1;
}

/* ---------------------------------------------------------------- um arquivo */
static int handle_client(int cs, const char *dir, int little)
{
    unsigned char hdr[HDR_SIZE];
    char name[NAME_LEN], path[160];
    uint32_t size, crc_in, crc = 0xFFFFFFFFUL, done = 0;
    long h, n;
    int ok = 0;

    if (read_full(cs, hdr, HDR_SIZE) != HDR_SIZE) {
        printf("cabecalho incompleto (conexao fechada ou %lu s sem receber nada)\n", RCV_TIMEOUT_MS / 1000);
        return -1;
    }
    size   = little ? le32(hdr)     : be32(hdr);
    crc_in = little ? le32(hdr + 4) : be32(hdr + 4);
    memcpy(name, hdr + 8, NAME_LEN);
    name[NAME_LEN - 1] = '\0';

    printf("arquivo '%s', %lu bytes\n", name, (unsigned long)size);
    if (!name_ok(name)) { printf("nome invalido\n"); return -1; }
    if (size > MAX_SIZE) {
        printf("tamanho absurdo: confira a ordem dos bytes (opcao -l = little-endian)\n");
        return -1;
    }

    if (dir && dir[0]) {
        size_t dl = strlen(dir);
        if (dl + 1 + strlen(name) + 1 > sizeof path) { printf("pasta muito longa\n"); return -1; }
        sprintf(path, "%s%s%s", dir, (dir[dl - 1] == '/') ? "" : "/", name);
    } else {
        strcpy(path, name);
    }

    h = rigel_ff_open(path, RIGEL_FA_WRITE | RIGEL_FA_CREATE_ALWAYS);
    if (h < 0) { printf("nao consegui criar %s\n", path); return -1; }

    while (done < size) {
        long want = (size - done > BUF_SIZE) ? BUF_SIZE : (long)(size - done);
        n = read(cs, buf, (size_t)want);
        if (n <= 0) break;                                      /* fechou, erro ou tempo esgotado */
        if (rigel_ff_write(h, buf, n) != n) { printf("erro ao gravar (disco cheio?)\n"); break; }
        if (crc_in) crc = crc_update(crc, buf, n);
        if ((done >> 15) != ((done + (uint32_t)n) >> 15)) { putchar('.'); fflush(stdout); }
        done += (uint32_t)n;
    }
    if (done) putchar('\n');

    if (rigel_ff_close(h) < 0) printf("erro ao fechar o arquivo\n");
    else if (done == size) ok = 1;

    if (!ok && done != size)
        printf("recebi so' %lu de %lu bytes (se o tamanho parece errado, tente a opcao -l)\n",
               (unsigned long)done, (unsigned long)size);

    if (ok && crc_in) {
        crc ^= 0xFFFFFFFFUL;
        if (crc != crc_in) {
            printf("CRC errado: veio %08lX, calculei %08lX\n", (unsigned long)crc_in, (unsigned long)crc);
            ok = 0;
        }
    }

    if (!ok) { unlink(path); printf("arquivo %s apagado\n", path); return -1; }
    printf("gravado %s (%lu bytes)\n", path, (unsigned long)size);
    return 0;
}

/* ---------------------------------------------------------------- servidor */
int main(int argc, char *argv[])
{
    rigel_sockaddr_in a, peer;
    const char *dir = NULL;
    unsigned long port = 0;
    int little = 0, once = 0, i, npos = 0;
    uint32_t one = 1, tmo = (uint32_t)RCV_TIMEOUT_MS;
    long ls, cs, plen;
    char ipb[20];

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-l") == 0)      little = 1;
        else if (strcmp(argv[i], "-1") == 0) once = 1;
        else if (npos == 0) { port = strtoul(argv[i], NULL, 10); npos++; }
        else if (npos == 1) { dir = argv[i]; npos++; }
    }
    if (port == 0 || port > 65535) {
        printf("uso: rxfile [-l] [-1] PORTA [PASTA]\n");
        return 1;
    }
    crc_init();

    ls = rigel_socket(RIGEL_AF_INET, RIGEL_SOCK_STREAM, 0);
    if (ls < 0) { printf("socket falhou (a rede esta iniciada?)\n"); return 1; }
    rigel_setsockopt(ls, RIGEL_SOL_SOCKET, RIGEL_SO_REUSEADDR, &one, sizeof one);
    memset(&a, 0, sizeof a);
    a.sin_family = RIGEL_AF_INET;
    a.sin_port   = swap_if_le16((uint16_t)port);
    a.sin_addr   = RIGEL_INADDR_ANY;
    if (rigel_bind(ls, &a, sizeof a) < 0) { printf("bind falhou\n"); return 1; }
    if (rigel_listen(ls, 2) < 0)          { printf("listen falhou\n"); return 1; }
    printf("rxfile: esperando arquivos na porta %lu%s%s\n", port,
           little ? " (little-endian)" : "", dir ? "" : " (pasta atual)");

    for (;;) {
        plen = sizeof peer;
        cs = rigel_accept(ls, &peer, &plen);
        if (cs < 0) { printf("accept falhou\n"); break; }
        ip_str(peer.sin_addr, ipb);
        printf("cliente %s:%u\n", ipb, (unsigned)swap_if_le16(peer.sin_port));
        if (rigel_setsockopt(cs, RIGEL_SOL_SOCKET, RIGEL_SO_RCVTIMEO, &tmo, sizeof tmo) < 0)
            printf("aviso: nao consegui ligar o limite de tempo (setsockopt falhou)\n");
        handle_client((int)cs, dir, little);
        close((int)cs);
        if (once) break;
    }
    close((int)ls);
    return 0;
}
