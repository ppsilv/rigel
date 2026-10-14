#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/vfs.h>
#include <duart.h>

#define RTC_BASE   0xFF9100UL

#define reg00   (*(volatile uint8_t *)(RTC_BASE + 0x01))



