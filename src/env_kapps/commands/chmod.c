/* ============================================================
 * chmod.c - Gerenciador de atributos de arquivo (FAT) no Rigel
 * (portado do chmod do oriondos, Projeto Orion68)
 *
 * Uso:
 *   chmod arquivo.ext                 -> mostra os atributos atuais
 *   chmod +r -h -s arquivo.ext        -> liga RDO, desliga HID e SYS
 *   chmod +r+h+s arquivo.ext          -> tambem aceita colado
 *
 * Flags suportadas (mesma letra do DOS):
 *   r = RDO (somente leitura)
 *   h = HID (oculto)
 *   s = SYS (sistema)
 *   a = ARC (arquivo - "modificado desde o ultimo backup")
 *   d = bit 6 (dispositivo)
 *   l = bit 7 (link)
 *
 * A coluna mostrada e' DLRHSA. Para gravar d e l o kernel (sys_fsfile.c) e o f_chmod do
 * FatFS precisam deixar passar os bits 6 e 7 (use o patch_chmod_dl.py).
 * ============================================================ */

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <rigel/rigel.h>

/* bits 6 e 7 do byte de atributo (iguais aos do ls) */
#ifndef RIGEL_ATTR_DEV
#define RIGEL_ATTR_DEV   0x40            /* D = dispositivo */
#endif
#ifndef RIGEL_ATTR_LNK
#define RIGEL_ATTR_LNK   0x80            /* L = link */
#endif

typedef struct {
    char    letra;
    uint8_t bit;
} attr_map_t;

static const attr_map_t ATTR_TABLE[] = {
    { 'd', RIGEL_ATTR_DEV },
    { 'l', RIGEL_ATTR_LNK },
    { 'r', RIGEL_ATTR_RDO },
    { 'h', RIGEL_ATTR_HID },
    { 's', RIGEL_ATTR_SYS },
    { 'a', RIGEL_ATTR_ARC },
};
#define ATTR_TABLE_LEN  ((int)(sizeof ATTR_TABLE / sizeof ATTR_TABLE[0]))

/* "DLRHSA": uma letra por atributo, '-' quando nao tem (mesmo formato do ls) */
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

/* Mostra os atributos atuais de um arquivo. Retorna 0 em sucesso. */
static int show_attr(const char *path)
{
    rigel_dirent_t e;
    char buf[7];

    if (rigel_stat((char *)path, &e) != 0) {
        printf("chmod: nao foi possivel acessar '%s'\n", path);
        return 1;
    }

    fmt_attr_string(e.attr, buf);
    printf("%s  %s\n", buf, path);
    return 0;
}

/* Interpreta uma string tipo "+r-h+s" e preenche 'set_mask'
 * (bits a ligar) e 'clr_mask' (bits a desligar).
 * Retorna 0 se a string era valida (so +/- seguidos de r h s a d l). */
static int parse_flag_arg(const char *arg, uint8_t *set_mask, uint8_t *clr_mask)
{
    char sinal = 0;
    int i;

    while (*arg) {
        char c;

        if (*arg == '+' || *arg == '-') {
            sinal = *arg;
            arg++;
            continue;
        }

        if (sinal == 0)
            return 1;                    /* letra sem +/- antes: argumento invalido */

        c = *arg;
        if (c >= 'A' && c <= 'Z')        /* aceita maiuscula tambem */
            c = (char)(c - 'A' + 'a');

        for (i = 0; i < ATTR_TABLE_LEN; i++) {
            if (c == ATTR_TABLE[i].letra) {
                if (sinal == '+')
                    *set_mask |= ATTR_TABLE[i].bit;
                else
                    *clr_mask |= ATTR_TABLE[i].bit;
                break;
            }
        }
        if (i == ATTR_TABLE_LEN)
            return 1;                    /* letra desconhecida */

        arg++;
    }

    return 0;
}

int main(int argc, char *argv[])
{
    uint8_t set_mask = 0;
    uint8_t clr_mask = 0;
    const char *path = NULL;
    rigel_dirent_t e;
    uint8_t novo;
    int i;

    if (argc < 2) {
        printf("Uso: chmod [+r|-r] [+h|-h] [+s|-s] [+a|-a] [+d|-d] [+l|-l] arquivo\n");
        printf("     chmod arquivo            (mostra atributos atuais)\n");
        return 1;
    }

    /* Separa os argumentos de flag do argumento de path.
     * Convencao: qualquer arg comecando com + ou - e' flag,
     * o resto e' o caminho do arquivo (so aceita um path por vez). */
    for (i = 1; i < argc; i++) {
        if (argv[i][0] == '+' || argv[i][0] == '-') {
            if (parse_flag_arg(argv[i], &set_mask, &clr_mask)) {
                printf("chmod: argumento invalido '%s'\n", argv[i]);
                return 1;
            }
        } else {
            if (path != NULL) {
                printf("chmod: mais de um arquivo especificado\n");
                return 1;
            }
            path = argv[i];
        }
    }

    if (path == NULL) {
        printf("chmod: nenhum arquivo especificado\n");
        return 1;
    }

    /* Sem nenhuma flag: so mostra os atributos atuais */
    if (set_mask == 0 && clr_mask == 0)
        return show_attr(path);

    /* set_mask liga bits, clr_mask desliga -- nao podem se sobrepor
     * (ex: "+r-r" na mesma chamada nao faz sentido) */
    if (set_mask & clr_mask) {
        printf("chmod: flag conflitante (+ e - para o mesmo atributo)\n");
        return 1;
    }

    /* A syscall grava os 6 atributos de uma vez: le os atuais, aplica as mudancas e grava. */
    if (rigel_stat((char *)path, &e) != 0) {
        printf("chmod: nao foi possivel acessar '%s'\n", path);
        return 1;
    }
    novo = (uint8_t)((e.attr | set_mask) & (uint8_t)~clr_mask);
    novo &= (RIGEL_ATTR_DEV | RIGEL_ATTR_LNK | RIGEL_ATTR_RDO | RIGEL_ATTR_HID | RIGEL_ATTR_SYS | RIGEL_ATTR_ARC);

    if (rigel_chmod((char *)path, novo) != 0) {
        printf("chmod: erro ao alterar atributo(s) de '%s'\n", path);
        return 1;
    }

    return show_attr(path);
}
