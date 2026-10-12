/*
 * teste_trap.c -- 3o teste: isola o TRAP #0 (sem newlib, sem librigel).
 * 1) mostra para onde o vetor do TRAP #0 aponta (antes de disparar);
 * 2) so' depois dispara  trap #0  com D0 = SYS_GETPID (0x44).
 * Todo o texto vai direto na DUART.
 *
 * Se aparecer "vet32..." e o programa travar depois de "disparando trap #0",
 * o problema esta' entre o vetor e o handler do kernel (nao na lib).
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
    while (*s) { if (*s == '\n') putc_raw('\r'); putc_raw(*s++); }
}
static void hex32(unsigned long v)
{
    static const char h[] = "0123456789ABCDEF";
    for (int i = 28; i >= 0; i -= 4) putc_raw(h[(v >> i) & 0xF]);
}
static void show(const char *nome, unsigned long v)
{
    puts_raw(nome); puts_raw(" = 0x"); hex32(v); puts_raw("\n");
}

void exit(int c) { (void)c; for (;;) ; }   /* o crt0_app chama exit */

int main(void)
{
    /* vetor 32 (TRAP #0) fica no endereco 32*4 = 0x80 */
    unsigned long vet = *(volatile unsigned long *)0x80;
    show("vet32 (end 0x80)     ", vet);

    /* primeiras 3 palavras de 32 bits no destino do vetor */
    volatile unsigned long *p = (volatile unsigned long *)vet;
    show("  [destino+0]        ", p[0]);
    show("  [destino+4]        ", p[1]);
    show("  [destino+8]        ", p[2]);

    puts_raw("disparando trap #0 (getpid)\n");
#ifdef __m68k__
    register long d0 __asm__("d0") = 0x44;
    __asm__ volatile ("trap #0" : "+r"(d0) : : "d1", "a0", "a1", "memory", "cc");
    show("voltou, d0           ", (unsigned long)d0);
#endif

    puts_raw("T3: fim\n");
    for (;;)
        ;
    return 0;
}