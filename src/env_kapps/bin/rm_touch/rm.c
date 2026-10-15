/*
 * rmfile.c -- apaga um arquivo (mostra o stat antes e confirma depois).
 *
 *   rmfile ARQUIVO
 */
#include <stdio.h>
#include <unistd.h>      /* unlink */
#include <rigel/rigel.h>

int main(int argc, char *argv[])
{
    rigel_dirent_t e;

    if (argc < 2) {
        printf("uso: rmfile ARQUIVO\n");
        return 1;
    }
    if (rigel_stat(argv[1], &e) != 0) {
        printf("%s: nao existe\n", argv[1]);
        return 1;
    }
    printf("apagando %s (%lu bytes, %04u-%02u-%02u %02u:%02u:%02u)\n", argv[1],
           (unsigned long)e.size,
           1980 + (e.date >> 9), (e.date >> 5) & 15, e.date & 31,
           e.time >> 11, (e.time >> 5) & 63, (e.time & 31) * 2);

    if (unlink(argv[1]) != 0) {
        printf("erro ao apagar\n");
        return 1;
    }
    printf("%s: %s\n", argv[1],
           rigel_stat(argv[1], &e) != 0 ? "apagado" : "AINDA EXISTE");
    return 0;
}
