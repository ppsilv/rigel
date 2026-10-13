#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <malloc.h>
#include <ctype.h>
#include <string.h>
#include "shell.h"
#include <rigel/rigel.h>

extern const cmd_entry_t g_cmd_table[];

void do_help(int argc, char *argv[])
{
	int i = 0;

	printf("\nAvailable commands:\n");

	while (g_cmd_table[i].name)
    {
		printf("%12s - %s\n", g_cmd_table[i].name, g_cmd_table[i].helpme);
		++i;
	}
}

void do_load(int argc, char *argv[]){
//No shell, use assim:
    char *args[] = { "hello.elf", "ola", "mundo", NULL };
    long pid = rigel_spawn("hello.elf", args);
    printf("spawn -> pid=%ld\n", pid);
    if (pid > 0) {
       // rigel_waitpid(pid);
       // printf("filho %ld terminou\n", pid);
    }
    printf("Commando voltou antes do filho terminar...\n");
/*
O teste_spawn.c do pacote faz isso, lançando um hello.elf e esperando por ele.

Limites:

No máximo 16 argumentos e 1 KB de texto; passando disso o spawn devolve -1.
Só um spawn roda por vez (há um flag de bloqueio).
O waitpid espera em laço ocupado e devolve sempre 0, sem o código de saída do filho.
O resto do kernel continua sem proteção contra duas tasks usando o FatFS ao mesmo tempo. Se o filho abrir arquivos enquanto o shell também lê o SD, pode dar problema.
*/

}