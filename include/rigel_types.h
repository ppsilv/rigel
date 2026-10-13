#ifndef __RIGEL_H__
#define __RIGEL_H__



typedef unsigned int    UINT;   /* int must be 16-bit or 32-bit */
typedef unsigned char   BYTE;   /* char must be 8-bit */
typedef unsigned short  WORD;   /* short must be 16-bit */
typedef unsigned long   DWORD;  /* long must be 32-bit */
typedef WORD            WCHAR;  /* UTF-16 code unit */

typedef DWORD           FSIZE_t;


typedef struct {
    uint8_t sec;
    uint8_t min;
    uint8_t hour;
    uint8_t mday;
    uint8_t mon;
    uint8_t year; /* 00-99 */
} __attribute__((packed))  rtc_time_t;

extern void sys_rtc_read_time(rtc_time_t *t);
extern void sys_rtc_set_time(rtc_time_t *t);
extern uint8_t sys_eeprom_read_byte(uint16_t addr);
extern void sys_eeprom_write_byte(uint16_t addr, uint8_t data);



#endif