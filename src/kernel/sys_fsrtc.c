/*
 * sys_fsrtc.c -- SYS_READRTC / SYS_WRITERTC do kernel Rigel (RTC DS1307 no Pico 2 W).
 *
 *   long sys_readrtc(void)       -> timestamp de 32 bits (formato do rtc_to_timestamp do Pico)
 *   long sys_writertc(long ts)   -> 0   (acerta o relogio com o timestamp)
 *
 * Interface com o Pico: byte unico em enderecos IMPARES da janela 0xFF9100..0xFF917F.
 * O Pico decodifica os bits A6..A1 do endereco do 68000 como o "case":
 *
 *      endereco = 0xFF9100 + (case << 1) + 1
 *
 *      case 0x0E  0xFF911D  ESCRITA  byte 3 (mais significativo) do timestamp
 *      case 0x0F  0xFF911F  ESCRITA  byte 2
 *      case 0x10  0xFF9121  ESCRITA  byte 1
 *      case 0x11  0xFF9123  ESCRITA  byte 0  -> o Pico grava no DS1307 neste momento
 *      case 0x12  0xFF9125  LEITURA  byte 3  -> o Pico le o DS1307 neste momento
 *      case 0x13  0xFF9127  LEITURA  byte 2
 *      case 0x14  0xFF9129  LEITURA  byte 1
 *      case 0x15  0xFF912B  LEITURA  byte 0
 */
#include <stdint.h>

#define RTC_WINDOW      0x00FF9100UL
#define RTC_ADDR(c)     (RTC_WINDOW + ((unsigned long)(c) << 1) + 1UL)

#define RTC_CASE_W_B3   0x0E
#define RTC_CASE_W_B2   0x0F
#define RTC_CASE_W_B1   0x10
#define RTC_CASE_W_B0   0x11
#define RTC_CASE_R_B3   0x12
#define RTC_CASE_R_B2   0x13
#define RTC_CASE_R_B1   0x14
#define RTC_CASE_R_B0   0x15

#ifdef RTC_HOST_TEST                          /* so' para teste no PC */
extern uint8_t rtc_test_rd(int c);
extern void    rtc_test_wr(int c, uint8_t v);
#define RTC_RD(c)       rtc_test_rd(c)
#define RTC_WR(c, v)    rtc_test_wr((c), (v))
#else
#define RTC_RD(c)       (*(volatile uint8_t *)RTC_ADDR(c))
#define RTC_WR(c, v)    (*(volatile uint8_t *)RTC_ADDR(c) = (uint8_t)(v))
#endif

#if defined(__m68k__) && !defined(RTC_HOST_TEST)
static inline uint16_t irq_off(void)
{
    uint16_t sr;
    __asm__ volatile ("move.w %%sr,%0\n\tor.w #0x0700,%%sr" : "=d"(sr) : : "memory", "cc");
    return sr;
}
static inline void irq_back(uint16_t sr) { __asm__ volatile ("move.w %0,%%sr" : : "d"(sr) : "memory", "cc"); }
#else
static inline uint16_t irq_off(void) { return 0; }
static inline void irq_back(uint16_t sr) { (void)sr; }
#endif

#ifndef RTC_WAIT_LOOPS
#define RTC_WAIT_LOOPS  4000UL    /* ~3 a 5 ms num 68000 de poucos MHz; aumente se a leitura vier velha */
#endif

static void rtc_wait(void)
{
    volatile unsigned long n = RTC_WAIT_LOOPS;
    while (n--) { }
}

/*
 * Tudo com interrupcao mascarada: a interrupcao do teclado tambem fala com o
 * Pico (0xFF9113) e bagunçaria a sequencia.
 */
long sys_readrtc(void)
{
    uint32_t ts;
    uint16_t sr = irq_off();

    /*
     * O acesso ao case 0x12 DISPARA a leitura do DS1307 no Pico (~1 ms de I2C),
     * mas o byte que volta nesse mesmo acesso ainda e' o valor ANTIGO.
     * Por isso: 1) leitura "de disparo" (valor descartado); 2) espera;
     * 3) le b2,b1,b0 (ja prontos); 4) le b3 por ultimo (devolve o b3 da
     * amostra do passo 1; o novo disparo que ele causa e' ignorado).
     */
    (void)RTC_RD(RTC_CASE_R_B3);
    rtc_wait();

    ts  = (uint32_t)RTC_RD(RTC_CASE_R_B2) << 16;
    ts |= (uint32_t)RTC_RD(RTC_CASE_R_B1) <<  8;
    ts |= (uint32_t)RTC_RD(RTC_CASE_R_B0);
    ts |= (uint32_t)RTC_RD(RTC_CASE_R_B3) << 24;

    irq_back(sr);
    return (long)ts;
}

long sys_writertc(long rtc)
{
    uint32_t ts = (uint32_t)rtc;
    uint16_t sr = irq_off();

    RTC_WR(RTC_CASE_W_B3, ts >> 24);
    RTC_WR(RTC_CASE_W_B2, ts >> 16);
    RTC_WR(RTC_CASE_W_B1, ts >>  8);
    RTC_WR(RTC_CASE_W_B0, ts);                       /* este acesso faz o Pico gravar no DS1307 */

    irq_back(sr);
    return 0;
}
