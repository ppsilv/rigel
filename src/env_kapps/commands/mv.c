/* ============================================================
 * mv.c - move/renomeia arquivos e diretorios (Rigel)
 *
 * Uso:   mv [-f] origem destino        renomeia/move origem para destino
 *        mv [-f] origem... diretorio   move varias origens para dentro do diretorio
 *
 *   -f   se o destino ja existir e for um arquivo, apaga o destino antes de mover
 *        (sem -f o mv recusa e avisa, o FAT nao sobrescreve num rename)
 *
 * Usa a syscall rename (SYS_FRENAME -> f_rename do FatFS): mantem os atributos
 * (inclusive D e L) e so' funciona dentro do mesmo volume.
 * ============================================================ */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>          /* unlink */
#include <rigel/rigel.h>

#define PATHMAX 256

static int is_dir(const char *path)
{
    rigel_dirent_t e;
    return rigel_stat((char *)path, &e) == 0 && (e.attr & RIGEL_ATTR_DIR);
}
static int exists(const char *path)
{
    rigel_dirent_t e;
    return rigel_stat((char *)path, &e) == 0;
}

/* ultimo componente do caminho (ignora '/' no fim) -> copia para 'out' */
static int base_name(const char *path, char *out, size_t outsz)
{
    size_t len = strlen(path), end = len, start;
    while (end > 1 && path[end - 1] == '/') end--;
    start = end;
    while (start > 0 && path[start - 1] != '/') start--;
    if (end - start == 0 || end - start >= outsz) return -1;
    memcpy(out, path + start, end - start);
    out[end - start] = '\0';
    return 0;
}

static int move_one(const char *src, const char *dst, int force)
{
    size_t ls;

    if (!exists(src)) { printf("mv: %s: nao existe\n", src); return 1; }
    if (strcmp(src, dst) == 0) { printf("mv: origem e destino sao o mesmo: %s\n", src); return 1; }

    /* diretorio dentro dele mesmo ("mv a a/b") */
    ls = strlen(src);
    if (is_dir(src) && strncmp(src, dst, ls) == 0 && dst[ls] == '/') {
        printf("mv: nao da para mover %s para dentro dele mesmo\n", src);
        return 1;
    }

    if (exists(dst)) {
        if (is_dir(dst) || is_dir(src) || !force) {
            printf("mv: %s: destino ja existe%s\n", dst,
                   (is_dir(dst) || is_dir(src)) ? "" : " (use -f para sobrescrever)");
            return 1;
        }
        if (unlink(dst) != 0) { printf("mv: %s: nao foi possivel apagar o destino\n", dst); return 1; }
    }

    if (rename(src, dst) != 0) {
        printf("mv: falhou ao mover %s para %s (volumes diferentes ou caminho invalido?)\n", src, dst);
        return 1;
    }
    return 0;
}

int main(int argc, char **argv)
{
    int  force = 0, i = 1, nsrc, rc = 0;
    const char *dest;
    char path[PATHMAX], name[PATHMAX];

    if (i < argc && strcmp(argv[i], "-f") == 0) { force = 1; i++; }
    nsrc = argc - i - 1;
    if (nsrc < 1) {
        printf("Uso: mv [-f] origem destino\n     mv [-f] origem... diretorio\n");
        return 1;
    }
    dest = argv[argc - 1];

    if (is_dir(dest)) {                                   /* mv a b c dir/ */
        size_t ld = strlen(dest);
        for (; i < argc - 1; i++) {
            if (base_name(argv[i], name, sizeof name) != 0) { printf("mv: nome invalido: %s\n", argv[i]); rc = 1; continue; }
            if (ld + 1 + strlen(name) >= sizeof path)      { printf("mv: caminho muito longo\n"); rc = 1; continue; }
            strcpy(path, dest);
            if (ld > 0 && path[ld - 1] != '/') strcat(path, "/");
            strcat(path, name);
            if (move_one(argv[i], path, force) != 0) rc = 1;
        }
        return rc;
    }

    if (nsrc > 1) { printf("mv: %s: nao e' um diretorio\n", dest); return 1; }
    return move_one(argv[i], dest, force);
}
