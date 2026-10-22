/* ============================================================
 * ls.c - Lista arquivos do diretorio (FAT) no Rigel
 * (portado do ls do oriondos, Projeto Orion68)
 *
 * Uso:
 *   ls                 -> lista o diretorio atual, sem ocultos/sistema
 *   ls -a              -> lista tudo, incluindo HID e SYS
 *   ls -w              -> lista so os nomes, em colunas (estilo DOS /w)
 *   ls -t              -> ordena pelo nome (sem diferenciar maiuscula/minuscula)
 *   ls -d              -> diretorios primeiro, arquivos depois
 *   ls -f              -> arquivos primeiro, diretorios depois
 *   ls caminho         -> lista o diretorio informado
 *   ls -a -w caminho   -> combina as opcoes
 *   ls -h              -> ajuda
 *
 * -d e -f agrupam mantendo a ordem original do disco dentro de cada
 * grupo; combine com -t para ordenar por nome dentro de cada grupo.
 * Se der -d e -f juntos, vale o ultimo.
 * ============================================================ */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <rigel/rigel.h>

/* Largura de coluna do modo -w e quantas colunas cabem numa tela
 * de 80 colunas (igual ao "dir /w" do DOS: 5 colunas de 16) */
#define WIDE_COL_WIDTH 16
#define WIDE_COLS      (80 / WIDE_COL_WIDTH)

/* bits 6 e 7 do byte de atributo (reservados no FAT; o Rigel os usa para o tipo do arquivo) */
#ifndef RIGEL_ATTR_DEV
#define RIGEL_ATTR_DEV   0x40            /* D = dispositivo */
#endif
#ifndef RIGEL_ATTR_LNK
#define RIGEL_ATTR_LNK   0x80            /* L = link */
#endif

/* Uma entrada guardada para poder ordenar. O nome e' copiado (malloc do
 * tamanho exato) para nao depender de como o rigel_dirent_t guarda o nome. */
typedef struct {
    char         *name;
    uint8_t       attr;
    unsigned long size;
} entry_t;

/* Opcoes de ordenacao (globais porque o qsort nao tem parametro de contexto) */
static int g_sort_name = 0;              /* -t */
static int g_group     = 0;              /* -d: 1 = diretorios antes, -f: -1 = arquivos antes */

/* "DLRHSA": uma letra por atributo, '-' quando nao tem */
static void fmt_attr_string(uint8_t attr, char *out)
{
    out[0] = (attr & RIGEL_ATTR_DEV) ? 'D' : '-';
    out[1] = (attr & RIGEL_ATTR_LNK) ? 'L' : '-';
    out[2] = (attr & RIGEL_ATTR_RDO) ? 'R' : '-';
    out[3] = (attr & RIGEL_ATTR_HID) ? 'H' : '-';
    out[4] = (attr & RIGEL_ATTR_SYS) ? 'S' : '-';
    out[5] = (attr & RIGEL_ATTR_ARC) ? 'A' : '-';
    out[6] = '\0';
}

/* Monta o nome pro modo -w: diretorios entre [colchetes], como o DOS faz.
 * Trunca se nao couber na coluna e ja preenche com espacos ate 'width'. */
static void fmt_wide_name(const entry_t *e, char *out, int width)
{
    int maxlen = width - 1;              /* deixa 1 espaco de respiro entre colunas */
    int len = (int)strlen(e->name);
    int total, i;

    if (e->attr & RIGEL_ATTR_DIR) {
        if (len > maxlen - 2) len = maxlen - 2;      /* espaco pros colchetes */
        out[0] = '[';
        memcpy(out + 1, e->name, (size_t)len);
        out[1 + len] = ']';
        total = 2 + len;
    } else {
        if (len > maxlen) len = maxlen;
        memcpy(out, e->name, (size_t)len);
        total = len;
    }
    for (i = total; i < width; i++)
        out[i] = ' ';
    out[width] = '\0';
}

/* Compara nomes sem diferenciar maiuscula/minuscula (FAT nao diferencia).
 * Se so a caixa difere, desempata com strcmp para a ordem ser deterministica. */
static int name_cmp(const char *a, const char *b)
{
    const unsigned char *x = (const unsigned char *)a;
    const unsigned char *y = (const unsigned char *)b;

    while (*x && *y) {
        int cx = tolower(*x), cy = tolower(*y);
        if (cx != cy)
            return cx - cy;
        x++;
        y++;
    }
    if (*x != *y)
        return tolower(*x) - tolower(*y);
    return strcmp(a, b);
}

/* Comparador do qsort sobre um vetor de PONTEIROS para entry_t.
 * 1) grupo (diretorio/arquivo) se -d ou -f
 * 2) nome se -t
 * 3) posicao original: como os ponteiros apontam para o vetor lido na
 *    ordem do disco, comparar enderecos deixa a ordenacao estavel. */
static int cmp_entries(const void *pa, const void *pb)
{
    const entry_t *a = *(const entry_t * const *)pa;
    const entry_t *b = *(const entry_t * const *)pb;

    if (g_group) {
        int da = (a->attr & RIGEL_ATTR_DIR) != 0;
        int db = (b->attr & RIGEL_ATTR_DIR) != 0;
        if (da != db)
            return (da > db) ? -g_group : g_group;   /* g_group=1: diretorio vem antes */
    }
    if (g_sort_name) {
        int c = name_cmp(a->name, b->name);
        if (c)
            return c;
    }
    return (a < b) ? -1 : (a > b) ? 1 : 0;
}

static void free_entries(entry_t *v, int n)
{
    int i;
    for (i = 0; i < n; i++)
        free(v[i].name);
    free(v);
}

static int list_dir(const char *path, int show_hidden, int wide)
{
    rigel_dirent_t e;
    entry_t *v = NULL;                   /* entradas na ordem do disco */
    entry_t **order = NULL;              /* ponteiros na ordem de exibicao */
    int n = 0, cap = 0;
    char attrbuf[7];
    char namebuf[WIDE_COL_WIDTH + 1];
    int col = 0;
    int i;
    long d;

    d = rigel_opendir((char *)path);
    if (d < 0) {
        printf("ls: nao foi possivel abrir '%s'\n", path);
        return 1;
    }

    /* 1) le o diretorio inteiro para a memoria */
    while (rigel_readdir(d, &e) == 1) {
        size_t len;
        char *copy;

        if (!show_hidden && (e.attr & (RIGEL_ATTR_HID | RIGEL_ATTR_SYS)))
            continue;                    /* pula ocultos/sistema, como o DOS sem /a */

        if (n == cap) {
            int newcap = cap ? cap * 2 : 64;
            entry_t *nv = (entry_t *)realloc(v, (size_t)newcap * sizeof(entry_t));
            if (nv == NULL)
                goto sem_memoria;
            v = nv;
            cap = newcap;
        }
        len = strlen(e.name);
        copy = (char *)malloc(len + 1);
        if (copy == NULL)
            goto sem_memoria;
        memcpy(copy, e.name, len + 1);

        v[n].name = copy;
        v[n].attr = (uint8_t)e.attr;
        v[n].size = (unsigned long)e.size;
        n++;
    }
    rigel_closedir(d);
    d = -1;

    /* 2) monta a ordem de exibicao e ordena se pediram */
    order = (entry_t **)malloc((size_t)(n ? n : 1) * sizeof(entry_t *));
    if (order == NULL)
        goto sem_memoria;
    for (i = 0; i < n; i++)
        order[i] = &v[i];
    if (n > 1 && (g_sort_name || g_group))
        qsort(order, (size_t)n, sizeof(entry_t *), cmp_entries);

    /* 3) imprime */
    for (i = 0; i < n; i++) {
        const entry_t *p = order[i];

        if (wide) {
            fmt_wide_name(p, namebuf, WIDE_COL_WIDTH);
            printf("%s", namebuf);
            col++;
            if (col == WIDE_COLS)
                col = 0;                 /* 5 x 16 = 80: a tela ja quebra a linha sozinha */
        } else {
            fmt_attr_string(p->attr, attrbuf);
            if (p->attr & RIGEL_ATTR_DIR)
                printf("%s  %10s  %s\n", attrbuf, "<DIR>", p->name);
            else
                printf("%s  %10lu  %s\n", attrbuf, p->size, p->name);
        }
    }

    if (wide && col != 0)
        printf("\n");                    /* fecha a ultima linha se nao deu redondo */

    printf("\n%d item(s)\n", n);
    free(order);
    free_entries(v, n);
    return 0;

sem_memoria:
    if (d >= 0)
        rigel_closedir(d);
    printf("ls: sem memoria\n");
    free(order);
    free_entries(v, n);
    return 1;
}

static void help(void)
{
    printf("--------------------------------------\n");
    printf("Usage: ls -a = lists hidden files.\n");
    printf("       ls -w = lists files in columns.\n");
    printf("       ls -t = sorts by name.\n");
    printf("       ls -d = directories first.\n");
    printf("       ls -f = files first.\n");
    printf("       ls -h = show help.\n");
    printf("       ls = lists files in ONE column.\n");
    printf("--------------------------------------\n");
}

int main(int argc, char *argv[])
{
    const char *path = ".";
    int show_hidden = 0;
    int wide = 0;
    int i;

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0) {
            help();
            return 0;
        } else if (strcmp(argv[i], "-a") == 0) {
            show_hidden = 1;
        } else if (strcmp(argv[i], "-w") == 0) {
            wide = 1;
        } else if (strcmp(argv[i], "-t") == 0) {
            g_sort_name = 1;
        } else if (strcmp(argv[i], "-d") == 0) {
            g_group = 1;                 /* diretorios antes */
        } else if (strcmp(argv[i], "-f") == 0) {
            g_group = -1;                /* arquivos antes */
        } else {
            path = argv[i];
        }
    }

    if (!wide)
        printf("DLRHSA  %10s  Nome\n", "Tamanho");
    return list_dir(path, show_hidden, wide);
}
