#include <stdio.h>
#include <sys/interrupt.h>
#include <sys/critical.h>
#include <sys/scheduler.h>

void sys_setramvector(long stub_addr, long handler_addr)
{
    uint8_t *p = (uint8_t *) stub_addr;

    *p++ = 0x4E;
    *p++ = 0xF9;   /* JMP absoluto de 32 bits */

    *p++ = (uint8_t)((handler_addr >> 24) & 0xFF);
    *p++ = (uint8_t)((handler_addr >> 16) & 0xFF);
    *p++ = (uint8_t)((handler_addr >> 8)  & 0xFF);
    *p++ = (uint8_t)( handler_addr        & 0xFF);
}