/* ============================================================
 * tail.c - mostra as ultimas linhas de um ou mais arquivos (Rigel)
 *
 * Uso:   tail [-n N] arquivo...        (padrao: 10 linhas)
 *        tail -N arquivo...            (o mesmo que -n N)
 * Com mais de um arquivo, cada um vem precedido de "==> nome <==".
 *
 * Le o arquivo de TRAS PARA FRENTE (lseek) em blocos de 512 bytes: so' toca no
 * final do arquivo, mesmo que ele seja grande. Um '\n' no ultimo byte nao conta.
 * ============================================================ */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <rigel/rigel.h>

#define BUFSZ 512
static char buf[BUFSZ];

static int tail_file(const char *path, long n)
{
    long h = rigel_ff_open((char *)path, RIGEL_FA_READ);
    long size, pos, blk, r, i, count = 0, start = 0;
    int  found = 0;

    if (h < 0) { printf("tail: %s: nao foi possivel abrir\n", path); return 1; }

    size = rigel_ff_lseek(h, 0, RIGEL_SEEK_END);
    if (size < 0) { printf("tail: %s: erro de posicionamento\n", path); rigel_ff_close(h); return 1; }
    if (size == 0) { rigel_ff_close(h); return 0; }

    /* acha o inicio das ultimas n linhas, andando de tras para frente */
    pos = size;
    while (pos > 0 && !found) {
        blk = (pos > BUFSZ) ? BUFSZ : pos;
        pos -= blk;
        if (rigel_ff_lseek(h, pos, RIGEL_SEEK_SET) != pos ||
            rigel_ff_read(h, buf, blk) != blk) {
            printf("tail: %s: erro de leitura\n", path);
            rigel_ff_close(h);
            return 1;
        }
        for (i = blk - 1; i >= 0; i--) {
            if (buf[i] == '\n' && (pos + i) != size - 1) {     /* o '\n' final nao conta */
                if (++count == n) { start = pos + i + 1; found = 1; break; }
            }
        }
    }

    /* imprime de 'start' ate o fim */
    if (rigel_ff_lseek(h, start, RIGEL_SEEK_SET) != start) {
        printf("tail: %s: erro de posicionamento\n", path);
        rigel_ff_close(h);
        return 1;
    }
    for (;;) {
        r = rigel_ff_read(h, buf, BUFSZ);
        if (r < 0) { printf("tail: %s: erro de leitura\n", path); rigel_ff_close(h); return 1; }
        if (r == 0) break;
        fwrite(buf, 1, (size_t)r, stdout);
    }
    rigel_ff_close(h);
    return 0;
}

int main(int argc, char **argv)
{
    long n = 10;
    int  i = 1, rc = 0, nfiles;

    while (i < argc && argv[i][0] == '-' && argv[i][1] != '\0') {
        if (argv[i][1] == 'n') {
            if (argv[i][2] != '\0')      n = atol(&argv[i][2]);
            else if (i + 1 < argc)       n = atol(argv[++i]);
            else { printf("tail: a opcao -n precisa de um numero\n"); return 1; }
        } else if (argv[i][1] >= '0' && argv[i][1] <= '9') {
            n = atol(&argv[i][1]);
        } else {
            printf("tail: opcao invalida: %s\n", argv[i]);
            return 1;
        }
        i++;
    }
    nfiles = argc - i;
    if (nfiles <= 0) {
        printf("Uso: tail [-n N] arquivo...   (padrao: 10 linhas)\n");
        return 1;
    }
    if (n < 0) n = 0;

    for (; i < argc; i++) {
        if (nfiles > 1) printf("%s==> %s <==\n", (i > argc - nfiles) ? "\n" : "", argv[i]);
        if (n > 0 && tail_file(argv[i], n) != 0) rc = 1;
    }
    return rc;
}
