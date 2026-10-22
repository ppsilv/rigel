/* ============================================================
 * cp.c - copia arquivos e diretorios (Rigel)
 *
 * Uso:   cp [-r] [-n] origem destino        copia origem para destino
 *        cp [-r] [-n] origem... diretorio   copia varias origens para dentro do diretorio
 *
 *   -r   copia diretorios inteiros (recursivo, ate' 4 niveis de profundidade)
 *   -n   nao sobrescreve arquivos que ja existam no destino (pula em silencio)
 *
 * - Sem -n, um arquivo de destino que ja existe e' SOBRESCRITO (como no Linux).
 * - Os atributos R, H, S, D e L da origem sao copiados para o destino.
 * - Se o destino ja existe, a copia vai primeiro para um arquivo temporario (CP~TMP.TMP,
 *   na pasta do destino) e so' depois troca o destino. Assim o destino antigo nao e'
 *   perdido se a gravacao falhar, e copiar um arquivo sobre ele mesmo ("cp a ./a")
 *   e' seguro: nunca zera a origem.
 * - Se a gravacao falhar no meio (cartao cheio), o arquivo incompleto e' apagado.
 * ============================================================ */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>         /* strcasecmp */
#include <unistd.h>          /* unlink */
#include <sys/stat.h>        /* mkdir */
#include <rigel/rigel.h>

#define PATHMAX    200
#define BUFSZ      4096
#define MAX_DEPTH  4         /* cada nivel segura 1 diretorio aberto (o kernel so' tem 8) */

#ifndef RIGEL_ATTR_DEV
#define RIGEL_ATTR_DEV   0x40
#endif
#ifndef RIGEL_ATTR_LNK
#define RIGEL_ATTR_LNK   0x80
#endif
#define ATTR_COPIA  (RIGEL_ATTR_RDO | RIGEL_ATTR_HID | RIGEL_ATTR_SYS | RIGEL_ATTR_DEV | RIGEL_ATTR_LNK)

static char buf[BUFSZ];
static int  opt_r = 0, opt_n = 0;

static int get_attr(const char *path, uint8_t *attr)
{
    rigel_dirent_t e;
    if (rigel_stat((char *)path, &e) != 0) return -1;
    *attr = e.attr;
    return 0;
}
static int is_dir(const char *path)
{
    uint8_t a;
    return get_attr(path, &a) == 0 && (a & RIGEL_ATTR_DIR);
}
static int exists(const char *path)
{
    uint8_t a;
    return get_attr(path, &a) == 0;
}

/* ultimo componente do caminho (ignora '/' no fim) */
static int base_name(const char *path, char *out, size_t outsz)
{
    size_t end = strlen(path), start;
    while (end > 1 && path[end - 1] == '/') end--;
    start = end;
    while (start > 0 && path[start - 1] != '/') start--;
    if (end - start == 0 || end - start >= outsz) return -1;
    memcpy(out, path + start, end - start);
    out[end - start] = '\0';
    return 0;
}

/* dir + "/" + nome -> out */
static int join(char *out, size_t outsz, const char *dir, const char *name)
{
    size_t ld = strlen(dir);
    if (ld + 1 + strlen(name) >= outsz) return -1;
    strcpy(out, dir);
    if (ld > 0 && out[ld - 1] != '/') strcat(out, "/");
    strcat(out, name);
    return 0;
}

/* pasta do caminho, com a '/' no fim ("" se nao tem) */
static void dir_of(const char *path, char *out, size_t outsz)
{
    size_t n = strlen(path);
    while (n > 0 && path[n - 1] != '/') n--;
    if (n >= outsz) n = 0;
    memcpy(out, path, n);
    out[n] = '\0';
}

static int copy_file(const char *src, const char *dst, uint8_t attr)
{
    long hs, hd, r, w;
    int  existe;
    char tmp[PATHMAX];
    const char *alvo;

    if (strcasecmp(src, dst) == 0) { printf("cp: %s: origem e destino sao o mesmo arquivo\n", src); return 1; }

    existe = exists(dst);
    if (existe) {
        if (is_dir(dst)) { printf("cp: %s: o destino e' um diretorio\n", dst); return 1; }
        if (opt_n) return 0;
        dir_of(dst, tmp, sizeof tmp);
        if (strlen(tmp) + 11 >= sizeof tmp) { printf("cp: %s: caminho muito longo\n", dst); return 1; }
        strcat(tmp, "CP~TMP.TMP");
        alvo = tmp;                                 /* grava no temporario e troca no fim */
    } else {
        alvo = dst;
    }

    hs = rigel_ff_open((char *)src, RIGEL_FA_READ);
    if (hs < 0) { printf("cp: %s: nao foi possivel abrir a origem\n", src); return 1; }
    hd = rigel_ff_open((char *)alvo, RIGEL_FA_WRITE | RIGEL_FA_CREATE_ALWAYS);
    if (hd < 0) {
        printf("cp: %s: nao foi possivel criar o destino\n", dst);
        rigel_ff_close(hs);
        return 1;
    }

    for (;;) {
        r = rigel_ff_read(hs, buf, BUFSZ);
        if (r < 0) { printf("cp: %s: erro de leitura\n", src); goto falha; }
        if (r == 0) break;
        w = rigel_ff_write(hd, buf, r);
        if (w != r) { printf("cp: %s: erro de escrita (cartao cheio?)\n", dst); goto falha; }
    }
    rigel_ff_close(hs);
    if (rigel_ff_close(hd) < 0) { printf("cp: %s: erro ao fechar o destino\n", dst); unlink(alvo); return 1; }

    if (existe) {                                   /* troca o destino antigo pelo novo */
        if (unlink(dst) != 0) {
            printf("cp: %s: nao foi possivel substituir o destino (somente leitura?)\n", dst);
            unlink(alvo);
            return 1;
        }
        if (rename(alvo, dst) != 0) {
            printf("cp: %s: nao foi possivel renomear; a copia ficou em %s\n", dst, alvo);
            return 1;
        }
    }

    if (attr & ATTR_COPIA)                          /* o arquivo novo nasce so' com A */
        rigel_chmod((char *)dst, (long)(attr & (ATTR_COPIA | RIGEL_ATTR_ARC)));
    return 0;

falha:
    rigel_ff_close(hs);
    rigel_ff_close(hd);
    unlink(alvo);                                   /* nao deixa arquivo pela metade */
    return 1;
}

static int copy_entry(const char *src, const char *dst, uint8_t attr, int depth)
{
    long d;
    rigel_dirent_t e;
    char s2[PATHMAX], d2[PATHMAX];
    size_t ls;
    int rc = 0;

    if (!(attr & RIGEL_ATTR_DIR))
        return copy_file(src, dst, attr);

    if (!opt_r) { printf("cp: %s e' um diretorio (use -r)\n", src); return 1; }
    if (depth >= MAX_DEPTH) { printf("cp: %s: diretorios fundos demais (maximo %d niveis, contando a origem)\n", src, MAX_DEPTH); return 1; }

    ls = strlen(src);
    if (strncasecmp(src, dst, ls) == 0 && (dst[ls] == '/' || dst[ls] == '\0')) {
        printf("cp: nao da para copiar %s para dentro dele mesmo\n", src);
        return 1;
    }

    if (exists(dst)) {
        if (!is_dir(dst)) { printf("cp: %s: ja existe e nao e' um diretorio\n", dst); return 1; }
    } else if (mkdir(dst, 0) != 0) {
        printf("cp: %s: nao foi possivel criar o diretorio\n", dst);
        return 1;
    }

    d = rigel_opendir((char *)src);
    if (d < 0) { printf("cp: %s: nao foi possivel abrir o diretorio\n", src); return 1; }

    while (rigel_readdir(d, &e) == 1) {
        if (strcmp(e.name, ".") == 0 || strcmp(e.name, "..") == 0) continue;
        if (join(s2, sizeof s2, src, e.name) != 0 || join(d2, sizeof d2, dst, e.name) != 0) {
            printf("cp: caminho muito longo: %s/%s\n", src, e.name);
            rc = 1;
            continue;
        }
        if (copy_entry(s2, d2, e.attr, depth + 1) != 0) rc = 1;
    }
    rigel_closedir(d);
    return rc;
}

static int copy_one(const char *src, const char *dst)
{
    uint8_t attr;
    if (get_attr(src, &attr) != 0) { printf("cp: %s: nao existe\n", src); return 1; }
    return copy_entry(src, dst, attr, 0);
}

int main(int argc, char **argv)
{
    int  i = 1, nsrc, rc = 0;
    const char *dest;
    char path[PATHMAX], name[PATHMAX];

    while (i < argc && argv[i][0] == '-' && argv[i][1] != '\0') {
        const char *o = &argv[i][1];
        for (; *o; o++) {
            if      (*o == 'r' || *o == 'R') opt_r = 1;
            else if (*o == 'n')              opt_n = 1;
            else { printf("cp: opcao invalida: -%c\n", *o); return 1; }
        }
        i++;
    }
    nsrc = argc - i - 1;
    if (nsrc < 1) {
        printf("Uso: cp [-r] [-n] origem destino\n     cp [-r] [-n] origem... diretorio\n");
        return 1;
    }
    dest = argv[argc - 1];

    if (is_dir(dest)) {                                   /* cp a b c dir/ */
        for (; i < argc - 1; i++) {
            if (base_name(argv[i], name, sizeof name) != 0 || join(path, sizeof path, dest, name) != 0) {
                printf("cp: nome ou caminho invalido: %s\n", argv[i]);
                rc = 1;
                continue;
            }
            if (copy_one(argv[i], path) != 0) rc = 1;
        }
        return rc;
    }

    if (nsrc > 1) { printf("cp: %s: nao e' um diretorio\n", dest); return 1; }
    return copy_one(argv[i], dest);
}
