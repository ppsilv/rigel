/*
 * fattime.c -- get_fattime() do FatFS para o kernel Rigel, usando o RTC DS1307
 *              (via sys_readrtc, que fala com o Pico 2 W).
 *
 * O FatFS chama get_fattime() sempre que cria um arquivo/diretorio, fecha ou
 * sincroniza um arquivo aberto para escrita, etc. Retorna a data/hora no formato FAT:
 *
 *      bits 31:25  ano desde 1980   bits 15:11  hora
 *      bits 24:21  mes (1..12)      bits 10:5   minuto
 *      bits 20:16  dia (1..31)      bits  4:0   segundos / 2
 *
 * O RTC guarda hora LOCAL e o FAT tambem, entao nao ha conversao de fuso.
 *
 * Para nao gastar ~1 ms de barramento com o Pico a cada chamada, le o RTC uma vez
 * e depois conta o tempo com o systemTick (10 ms). Ressincroniza a cada 60 s.
 * Se o RTC nao responde (timestamp 0 ou absurdo), usa o ultimo valor bom;
 * se nunca houve um, devolve 1980-01-01 00:00:00.
 */
#include <stdint.h>

extern long sys_readrtc(void);
extern volatile uint32_t systemTick;       /* +1 a cada 10 ms (scheduler.c) */

#define RTC_EPOCH_YEAR      1970           /* o Pico usa epoca Unix (confirmado: 1792015853 = 2026-10-14) */
#define TS_MIN_VALIDO       1577836800UL   /* 2020-01-01: abaixo disso, considera RTC invalido */
#define RESYNC_TICKS        6000UL         /* 60 s */

static uint32_t base_ts;                   /* timestamp (s) na ultima sincronizacao */
static uint32_t base_tick;                 /* systemTick nesse momento */
static int      synced;                    /* 0 = nunca sincronizou */

/* dias desde 1970-01-01 -> data civil (algoritmo de Howard Hinnant) */
static void civil_from_days(int32_t z, int *y, int *m, int *d)
{
    z += 719468;
    int32_t era = (z >= 0 ? z : z - 146096) / 146097;
    uint32_t doe = (uint32_t)(z - era * 146097);
    uint32_t yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    int32_t  yy  = (int32_t)yoe + era * 400;
    uint32_t doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    uint32_t mp  = (5 * doy + 2) / 153;
    *d = (int)(doy - (153 * mp + 2) / 5 + 1);
    *m = (int)(mp < 10 ? mp + 3 : mp - 9);
    *y = (int)(yy + (*m <= 2));
}

static uint32_t ts_to_fat(uint32_t ts)
{
    int y, m, d;
    civil_from_days((int32_t)(ts / 86400UL), &y, &m, &d);
    uint32_t s = ts % 86400UL;
    int hh = (int)(s / 3600), mm = (int)((s % 3600) / 60), ss = (int)(s % 60);

    y -= 1980;
    if (y < 0)   y = 0;
    if (y > 127) y = 127;
    return ((uint32_t)y << 25) | ((uint32_t)m << 21) | ((uint32_t)d << 16) |
           ((uint32_t)hh << 11) | ((uint32_t)mm << 5) | (uint32_t)(ss / 2);
}

uint32_t get_fattime(void)
{
    uint32_t now_tick = systemTick;

    if (!synced || (uint32_t)(now_tick - base_tick) >= RESYNC_TICKS) {
        uint32_t ts = (uint32_t)sys_readrtc();
        if (ts >= TS_MIN_VALIDO) {
            base_ts   = ts;
            base_tick = now_tick;
            synced    = 1;
        } else if (synced) {
            /* RTC nao respondeu: continua contando pelo tick; tenta de novo na proxima */
            base_ts  += (now_tick - base_tick) / 100UL;
            base_tick = now_tick - ((now_tick - base_tick) % 100UL);
        }
    }

    if (!synced)
        return (1UL << 21) | (1UL << 16);          /* 1980-01-01 00:00:00 */

    return ts_to_fat(base_ts + (now_tick - base_tick) / 100UL);
}
