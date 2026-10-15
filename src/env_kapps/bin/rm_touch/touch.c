/*
 * wfile.c -- grava um arquivo e mostra data/hora gravadas pelo FatFS.
 *
 *   wfile ARQUIVO [texto ...]     cria (ou sobrescreve) ARQUIVO com o texto
 *                                 (sem texto: grava "Teste do Rigel\n")
 *   wfile -s ARQUIVO              so' mostra o stat (tamanho, data, hora)
 */
#include <stdio.h>
#include <string.h>
#include <rigel/rigel.h>

static void mostra(const char *path)
{
    rigel_dirent_t e;
    if (rigel_stat(path, &e) != 0) {
        printf("%s: nao existe\n", path);
        return;
    }
    printf("%s: %lu bytes  %04u-%02u-%02u %02u:%02u:%02u\n", path,
           (unsigned long)e.size,
           1980 + (e.date >> 9), (e.date >> 5) & 15, e.date & 31,
           e.time >> 11, (e.time >> 5) & 63, (e.time & 31) * 2);
}

int main(int argc, char *argv[])
{
    char  texto[256];
    long  h, r;
    int   i;

    if (argc >= 3 && strcmp(argv[1], "-s") == 0) {
        mostra(argv[2]);
        return 0;
    }
    if (argc < 2) {
        printf("uso: wfile ARQUIVO [texto ...]\n     wfile -s ARQUIVO\n");
        return 1;
    }

    texto[0] = '\0';
    for (i = 2; i < argc; i++) {
        if (strlen(texto) + strlen(argv[i]) + 2 >= sizeof texto) break;
        if (i > 2) strcat(texto, " ");
        strcat(texto, argv[i]);
    }
    if (texto[0] == '\0') strcpy(texto, "Teste do Rigel");
    strcat(texto, "\n");

    h = rigel_ff_open(argv[1], RIGEL_FA_WRITE | RIGEL_FA_CREATE_ALWAYS);
    if (h < 0) {
        printf("erro ao criar %s (h=%ld)\n", argv[1], h);
        return 1;
    }
    r = rigel_ff_write(h, texto, strlen(texto));
    if (r != (long)strlen(texto)) {
        printf("erro na escrita (r=%ld)\n", r);
        rigel_ff_close(h);
        return 1;
    }
    rigel_ff_close(h);                   /* o FatFS grava a data/hora aqui */

    printf("gravado: ");
    mostra(argv[1]);
    return 0;
}
