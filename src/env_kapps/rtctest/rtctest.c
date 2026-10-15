/*
 * rtc_test.c -- testa SYS_READRTC / SYS_WRITERTC (DS1307 via Pico 2 W).
 *
 *   rtc_test                      le a hora 5 vezes (1 s entre as leituras) -> o relogio tem que andar 1 s por vez
 *   rtc_test set AAAA MM DD HH MM SS    acerta a hora, e le de volta para conferir
 *                                 ex.:  rtc_test set 2026 10 14 21 40 00
 *
 * O syscall troca um timestamp de 32 bits. ATENCAO: a conversao timestamp <-> data abaixo
 * assume a MESMA regra do rtc_to_timestamp/timestamp_to_rtc do seu Pico:
 * segundos desde 1970-01-01 00:00:00 (Unix), sem fuso. Se a data impressa nao bater com a
 * linha "YYYY/MM/DD hh:mm:ss" que o Pico mostra, o seu Pico usa outra epoca: mude RTC_EPOCH_YEAR
 * (so' aceita 1970 ou 2000 aqui) -- o timestamp cru e' sempre impresso tambem.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <rigel/rigel.h>

#ifndef RTC_EPOCH_YEAR
#define RTC_EPOCH_YEAR 1970
#endif

/* dias desde 1970-01-01 para uma data do calendario gregoriano */
static int32_t days_from_civil(int32_t y, unsigned m, unsigned d)
{
    y -= m <= 2;
    int32_t era = (y >= 0 ? y : y - 399) / 400;
    unsigned yoe = (unsigned)(y - era * 400);
    unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
    unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + (int32_t)doe - 719468;
}

static void civil_from_days(int32_t z, int *y, int *m, int *d)
{
    z += 719468;
    int32_t era = (z >= 0 ? z : z - 146096) / 146097;
    unsigned doe = (unsigned)(z - era * 146097);
    unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    int32_t  yy  = (int32_t)yoe + era * 400;
    unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    unsigned mp  = (5 * doy + 2) / 153;
    *d = (int)(doy - (153 * mp + 2) / 5 + 1);
    *m = (int)(mp < 10 ? mp + 3 : mp - 9);
    *y = (int)(yy + (*m <= 2));
}

static uint32_t to_timestamp(int y, int mo, int d, int h, int mi, int s)
{
    int32_t days = days_from_civil(y, (unsigned)mo, (unsigned)d);
    if (RTC_EPOCH_YEAR == 2000) days -= 10957;          /* 1970 -> 2000 */
    return (uint32_t)days * 86400u + (uint32_t)h * 3600u + (uint32_t)mi * 60u + (uint32_t)s;
}

static void show(const char *rotulo, uint32_t ts)
{
    uint32_t secs = ts % 86400u;
    int32_t days = (int32_t)(ts / 86400u);
    int y, m, d;
    if (RTC_EPOCH_YEAR == 2000) days += 10957;
    civil_from_days(days, &y, &m, &d);
    printf("%s ts=%lu (0x%08lX)  %04d/%02d/%02d %02lu:%02lu:%02lu\n", rotulo,
           (unsigned long)ts, (unsigned long)ts, y, m, d,
           (unsigned long)(secs / 3600), (unsigned long)((secs / 60) % 60), (unsigned long)(secs % 60));
}

int main(int argc, char **argv)
{
    if (argc == 8 && strcmp(argv[1], "set") == 0) {
        uint32_t ts = to_timestamp(atoi(argv[2]), atoi(argv[3]), atoi(argv[4]),
                                   atoi(argv[5]), atoi(argv[6]), atoi(argv[7]));
        show("gravando:", ts);
        rigel_writertc((long)ts);
        rigel_sleep(50);                       /* da' tempo ao Pico terminar a gravacao no DS1307 */
        uint32_t lido = (uint32_t)rigel_readrtc();
        show("lido:    ", lido);
        long dif = (long)(lido - ts);
        printf("diferenca = %ld s  %s\n", dif, (dif >= 0 && dif <= 2) ? "OK" : "FALHOU");
        return (dif >= 0 && dif <= 2) ? 0 : 1;
    }
    if (argc != 1) {
        printf("uso: rtc_test              (le 5 vezes)\n"
               "     rtc_test set AAAA MM DD HH MM SS\n");
        return 1;
    }

    uint32_t anterior = 0;
    int ok = 1;
    for (int i = 0; i < 5; i++) {
        uint32_t ts = (uint32_t)rigel_readrtc();
        show("leitura:", ts);
        if (i > 0) {
            long dif = (long)(ts - anterior);
            printf("   +%ld s desde a leitura anterior %s\n", dif, (dif >= 1 && dif <= 2) ? "OK" : "FALHOU");
            if (dif < 1 || dif > 2) ok = 0;
        }
        anterior = ts;
        rigel_sleep(1000);                     /* 1 s */
    }
    printf("rtc_test: %s\n", ok ? "TUDO OK" : "ALGO FALHOU");
    return ok ? 0 : 1;
}
