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
    
    setvbuf(stdout, NULL, _IONBF, 0);   /* sem buffer: saida aparece na hora */
    printf("\nRigel bootstrap 0.1 newlib for m68k\nMinimum hardware duart-disk0\n");
    printf("Qual o seu nome? ");
    if (fgets(nome, sizeof nome, stdin)){
        printf("Ola, %s", nome);
    }
    printf("malloc funcionando: ");
    void *p = __builtin_malloc(100);
    printf("%s\n", p ? "sim" : "nao");
    return 0;
}
