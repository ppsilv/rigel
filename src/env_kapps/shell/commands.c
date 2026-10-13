#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <malloc.h>
#include <ctype.h>
#include <string.h>
#include "shell.h"

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