#include <stdio.h>
#include <stdint.h>
#include "duarts.h"

uint32_t tick_count=0;
uint32_t systemTick=0;

uint8_t *WR_CHAR=(uint8_t *)0xFF912B;

int main(void)
{
    char nome[32];
    *WR_CHAR='A';
    duart_putc('B');
    //duart_init_canal_a();
    setvbuf(stdout, NULL, _IONBF, 0);   /* sem buffer: saida aparece na hora */

    printf("\nRigel 0.1 -- newlib  m68k\n");
    printf("Qual o seu nome? ");
    if (fgets(nome, sizeof nome, stdin))
        printf("Ola, %s", nome);

    printf("malloc funcionando: ");
    void *p = __builtin_malloc(100);
    printf("%s\n", p ? "sim" : "nao");
    return 0;
}
