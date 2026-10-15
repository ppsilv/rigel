/*
 * rtc_poke.c -- teste "cru" do barramento 68000 <-> Pico, SEM syscall e SEM macro.
 * Os enderecos estao escritos como literais para eliminar qualquer duvida.
 *
 *   rtc_poke        : dispara a leitura (0xFF9125), espera, le 0xFF9127/29/2B e 0xFF9125
 *   rtc_poke 25     : le UM byte em 0xFF9100 + (0x25) , 0x25 em hexa (so' enderecos impares)
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

static void espera(void)
{
    volatile unsigned long n = 20000UL;
    while (n--) { }
}

int main(int argc, char *argv[])
{
    printf("poke: iniciando %d\n",argc);
    if (argc >= 2) {
        unsigned long off  = strtoul(argv[1], NULL, 16);
        unsigned long addr = 0xFF9100UL + off;
        uint8_t v = *(volatile uint8_t *)addr;
        printf("le  0x%06lX = 0x%02X\n", addr, v);
        return 0;
    }

    volatile uint8_t *p25 = (volatile uint8_t *)0xFF9125UL;   /* case 0x12: b3 + dispara */
    volatile uint8_t *p27 = (volatile uint8_t *)0xFF9127UL;   /* case 0x13: b2 */
    volatile uint8_t *p29 = (volatile uint8_t *)0xFF9129UL;   /* case 0x14: b1 */
    volatile uint8_t *p2b = (volatile uint8_t *)0xFF912BUL;   /* case 0x15: b0 */

    uint8_t d = *p25;                 /* disparo (valor descartado) */
    espera();
    uint8_t b2 = *p27;
    uint8_t b1 = *p29;
    uint8_t b0 = *p2b;
    uint8_t b3 = *p25;

    printf("disparo 0xFF9125 = 0x%02X (descartado)\n", d);
    printf("b3=%02X b2=%02X b1=%02X b0=%02X\n", b3, b2, b1, b0);
    printf("ts = 0x%02X%02X%02X%02X\n", b3, b2, b1, b0);
    return 0;
}