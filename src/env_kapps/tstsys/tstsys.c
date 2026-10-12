/*
 * teste_sys.c -- 2o teste: usa a librigel (write/getpid -> TRAP #0).
 * A saida de diagnostico vai DIRETO na DUART, para aparecer mesmo que o
 * write()/printf() do kernel falhe.
 *
 *   make normal : so' write() e getpid()
 *   -DUSE_PRINTF: tambem chama printf() da newlib (e fflush)
 */
#include <stdio.h>
#include <unistd.h>

#define DUART_SRA  (*(volatile unsigned char *)0xFF9003)
#define DUART_THR  (*(volatile unsigned char *)0xFF9007)
#define TXRDY      0x04

static void putc_raw(char c)
{
    while (!(DUART_SRA & TXRDY))
        ;
    DUART_THR = (unsigned char)c;
}
static void puts_raw(const char *s)
{
    while (*s) { if (*s == '\n') putc_raw('\r'); putc_raw(*s++); }
}
static void hex32(unsigned long v)
{
    static const char h[] = "0123456789ABCDEF";
    for (int i = 28; i >= 0; i -= 4) putc_raw(h[(v >> i) & 0xF]);
}
static void show(const char *nome, long v)
{
    puts_raw(nome); puts_raw(" = 0x"); hex32((unsigned long)v); puts_raw("\n");
}

int main(void)
{
    puts_raw("T2: inicio\n");

    //show("getpid()", getpid());
    show("write() ", write(1, "SYS write ok\n", 13));

#ifdef USE_PRINTF
    puts_raw("T2: chamando printf\n");
    printf("printf ok\n");
    fflush(stdout);
    puts_raw("T2: printf voltou\n");
#endif

    puts_raw("T2: fim, retornando de main -> exit\n");
    return 0;
}