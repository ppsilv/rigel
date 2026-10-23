/* ============================================================
 * head.c - mostra as primeiras linhas de um ou mais arquivos (Rigel)
 *
 * Uso:   head [-n N] arquivo...        (padrao: 10 linhas)
 *        head -N arquivo...            (o mesmo que -n N)
 * Com mais de um arquivo, cada um vem precedido de "==> nome <==".
 * ============================================================ */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <rigel/rigel.h>

#define BUFSZ 512
static char buf[BUFSZ];

static int head_file(const char *path, long n)
{
    long h = rigel_ff_open((char *)path, RIGEL_FA_READ);
    long lines = 0, r, k, out;
    int  done = 0, rc = 0;

    if (h < 0) { printf("head: %s: nao foi possivel abrir\n", path); return 1; }

    while (!done) {
        r = rigel_ff_read(h, buf, BUFSZ);
        if (r < 0) { printf("head: %s: erro de leitura\n", path); rc = 1; break; }
        if (r == 0) break;                              /* fim do arquivo */
        out = r;
        for (k = 0; k < r; k++) {
            if (buf[k] == '\n' && ++lines >= n) { out = k + 1; done = 1; break; }
        }
        fwrite(buf, 1, (size_t)out, stdout);
    }
    rigel_ff_close(h);
    return rc;
}

int main(int argc, char **argv)
{
    long n = 10;
    int  i = 1, rc = 0, nfiles;

    while (i < argc && argv[i][0] == '-' && argv[i][1] != '\0') {
        if (argv[i][1] == 'n') {
            if (argv[i][2] != '\0')      n = atol(&argv[i][2]);
            else if (i + 1 < argc)       n = atol(argv[++i]);
            else { printf("head: a opcao -n precisa de um numero\n"); return 1; }
        } else if (argv[i][1] >= '0' && argv[i][1] <= '9') {
            n = atol(&argv[i][1]);
        } else {
            printf("head: opcao invalida: %s\n", argv[i]);
            return 1;
        }
        i++;
    }
    nfiles = argc - i;
    if (nfiles <= 0) {
        printf("Uso: head [-n N] arquivo...   (padrao: 10 linhas)\n");
        return 1;
    }
    if (n < 0) n = 0;

    for (; i < argc; i++) {
        if (nfiles > 1) printf("%s==> %s <==\n", (i > argc - nfiles) ? "\n" : "", argv[i]);
        if (n > 0 && head_file(argv[i], n) != 0) rc = 1;
    }
    return rc;
}
