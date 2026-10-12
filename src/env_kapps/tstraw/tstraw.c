/*
 * teste_raw.c -- app de teste SEM newlib e SEM syscall.
 * Escreve direto na DUART (polling) e fica num laco infinito.
 * Serve para separar: "o loader e o crt0 funcionam?" de "a newlib/syscall quebra?".
 */
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
    while (*s) {
        if (*s == '\n') putc_raw('\r');
        putc_raw(*s++);
    }
}

static void hex32(unsigned long v)
{
    static const char h[] = "0123456789ABCDEF";
    for (int i = 28; i >= 0; i -= 4)
        putc_raw(h[(v >> i) & 0xF]);
}

static int variavel_global = 1;

/* exit() so' existe aqui porque o crt0_app chama exit e nao linkamos a libc */
void exit(int c) { (void)c; for (;;) ; }

int main(void)
{
    unsigned long sp;
    __asm__ volatile ("move.l %%sp,%0" : "=r"(sp));

    puts_raw("RAW OK\n");
    puts_raw("main    = 0x"); hex32((unsigned long)&main);            puts_raw("\n");
    puts_raw("global  = 0x"); hex32((unsigned long)&variavel_global); puts_raw("\n");
    puts_raw("sp      = 0x"); hex32(sp);                              puts_raw("\n");

    for (;;)
        ;
    return 0;
}