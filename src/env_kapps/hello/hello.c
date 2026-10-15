/*
 * hello.c -- app de teste do Rigel.
 * Imprime os argumentos recebidos (argc/argv) e uma contagem, usando printf da newlib.
 */
#include <stdio.h>

int main(int argc, char **argv)
{
    printf("Ola do hello.elf!\n");
    printf("argc = %d\n", argc);
    for (int i = 0; i < argc; i++)
        printf("  argv[%d] = \"%s\"\n", i, argv[i]);

    for (int i = 1; i <= 3; i++)
        printf("contando %d\n", i);

    printf("hello.elf terminou\n");
    return 0;
}


