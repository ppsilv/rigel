/*
 * shell_time.c -- comandos "time" e "date" do shell do Rigel.
 *
 *   time    mostra a hora   (HH:MM:SS)
 *   date    mostra a data   (dia da semana DD/MM/AAAA)
 *
 * sys_readrtc devolve segundos desde 01/01/1970. O que aparece na tela e' o que esta no RTC
 * somado a RTC_TZ_HORAS:
 *   - RTC com a hora do Brasil (settime -z -3 ...):  RTC_TZ_HORAS = 0   (padrao)
 *   - RTC em UTC (settime sem -z):                   RTC_TZ_HORAS = -3
 */
#include <stdio.h>
#include <stdint.h>
#include <rigel/rigel.h>

#ifndef RTC_TZ_HORAS
#define RTC_TZ_HORAS   0
#endif

#define RTC_MIN_VALID  1700000000UL      /* abaixo disso o relogio nunca foi acertado */

static const char *const wday_name[7] = { "dom", "seg", "ter", "qua", "qui", "sex", "sab" };

typedef struct { int y, mo, d, h, mi, s, wd; } sh_dt_t;

/* le o RTC e separa em data e hora; devolve 0 se o relogio parece acertado, -1 se nao */
static int sh_now(sh_dt_t *r)
{
    uint32_t t = (uint32_t)rigel_readrtc();
    uint32_t shown = t + (uint32_t)(RTC_TZ_HORAS * 3600L);
    uint32_t days = shown / 86400UL, rem = shown % 86400UL;
    long z   = (long)days + 719468L;
    long era = z / 146097L;
    long doe = z - era * 146097L;
    long yoe = (doe - doe / 1460L + doe / 36524L - doe / 146096L) / 365L;
    long yy  = yoe + era * 400L;
    long doy = doe - (365L * yoe + yoe / 4L - yoe / 100L);
    long mp  = (5L * doy + 2L) / 153L;
    long dd  = doy - (153L * mp + 2L) / 5L + 1L;
    long mm  = (mp < 10) ? mp + 3 : mp - 9;

    if (mm <= 2) yy++;
    r->y  = (int)yy;  r->mo = (int)mm;  r->d = (int)dd;
    r->h  = (int)(rem / 3600UL);
    r->mi = (int)((rem % 3600UL) / 60UL);
    r->s  = (int)(rem % 60UL);
    r->wd = (int)((days + 4UL) % 7UL);               /* 01/01/1970 foi quinta-feira */
    return (t >= RTC_MIN_VALID) ? 0 : -1;
}

static void sh_not_set_warning(void)
{
    printf("(relogio ainda nao acertado: use settime)\n");
}

void do_time(int argc, char *argv[])
{
    sh_dt_t n;
    (void)argv;

    if (argc > 0) { printf("uso: time    (mostra a hora; para acertar use settime)\n"); return; }
    if (sh_now(&n) != 0) sh_not_set_warning();
    printf("%02d:%02d:%02d\n", n.h, n.mi, n.s);
}

void do_date(int argc, char *argv[])
{
    sh_dt_t n;
    (void)argv;

    if (argc > 0) { printf("uso: date    (mostra a data; para acertar use settime)\n"); return; }
    if (sh_now(&n) != 0) sh_not_set_warning();
    printf("%s %02d/%02d/%04d\n", wday_name[n.wd], n.d, n.mo, n.y);
}