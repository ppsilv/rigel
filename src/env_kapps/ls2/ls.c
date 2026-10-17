/* ============================================================
 * ls.c - Lista arquivos do diretorio (FAT) no Rigel
 * (portado do ls do oriondos, Projeto Orion68)
 *
 * Uso:
 *   ls                 -> lista o diretorio atual, sem ocultos/sistema
 *   ls -a              -> lista tudo, incluindo HID e SYS
 *   ls -w              -> lista so os nomes, em colunas (estilo DOS /w)
 *   ls caminho         -> lista o diretorio informado
 *   ls -a -w caminho   -> combina as opcoes
 *   ls -h              -> ajuda
 * ============================================================ */

#include <stdint.h>
#include <stdio.h>
#include <string.h>
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
static void fmt_wide_name(const rigel_dirent_t *e, char *out, int width)
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

static int list_dir(const char *path, int show_hidden, int wide)
{
    rigel_dirent_t e;
    char attrbuf[7];
    char namebuf[WIDE_COL_WIDTH + 1];
    int total_files = 0;
    int col = 0;
    long d;

    d = rigel_opendir((char *)path);
    if (d < 0) {
        printf("ls: nao foi possivel abrir '%s'\n", path);
        return 1;
    }

    while (rigel_readdir(d, &e) == 1) {
        if (!show_hidden && (e.attr & (RIGEL_ATTR_HID | RIGEL_ATTR_SYS)))
            continue;                    /* pula ocultos/sistema, como o DOS sem /a */

        if (wide) {
            fmt_wide_name(&e, namebuf, WIDE_COL_WIDTH);
            printf("%s", namebuf);
            col++;
            if (col == WIDE_COLS)
                col = 0;                 /* 5 x 16 = 80: a tela ja quebra a linha sozinha */
        } else {
            fmt_attr_string(e.attr, attrbuf);
            if (e.attr & RIGEL_ATTR_DIR)
                printf("%s  %10s  %s\n", attrbuf, "<DIR>", e.name);
            else
                printf("%s  %10lu  %s\n", attrbuf, (unsigned long)e.size, e.name);
        }
        total_files++;
    }

    rigel_closedir(d);

    if (wide && col != 0)
        printf("\n");                    /* fecha a ultima linha se nao deu redondo */

    printf("\n%d item(s)\n", total_files);
    return 0;
}

static void help(void)
{
    printf("--------------------------------------\n");
    printf("Usage: ls -a = lists hidden files.\n");
    printf("       ls -w = lists files in columns.\n");
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
        } else {
            path = argv[i];
        }
    }

    if (!wide)
        printf("DLRHSA  %10s  Nome\n", "Tamanho");
    return list_dir(path, show_hidden, wide);
}
