/* rtcraw.c -- mostra o valor cru de sys_readrtc */
#include <stdio.h>
#include <stdint.h>
#include <rigel/rigel.h>

int main(void)
{
    uint32_t v = (uint32_t)rigel_readrtc();
    int i;

    printf("hex     : 0x%08lX\n", (unsigned long)v);
    printf("decimal : %lu\n", (unsigned long)v);
    printf("bytes   : ");
    for (i = 3; i >= 0; i--)
        printf("%02X ", (unsigned)((v >> (i * 8)) & 0xFF));
    printf("\n");
    return 0;
}