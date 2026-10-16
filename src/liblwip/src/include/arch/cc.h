/* arch/cc.h -- lwIP no Rigel (68000, big-endian, newlib) */
#ifndef RIGEL_LWIP_CC_H
#define RIGEL_LWIP_CC_H

#include <stdint.h>

#ifndef BYTE_ORDER
#define BYTE_ORDER  BIG_ENDIAN
#endif
#define LWIP_ERRNO_STDINCLUDE 1     /* usa o <errno.h> da newlib */
#define LWIP_NO_CTYPE_H       1     /* lwIP usa as suas proprias isdigit/isxdigit (sem a tabela _ctype_ da newlib) */

extern void     kernel_puts(const char *s);
extern uint32_t rigel_rand32(void);

#define LWIP_RAND()  ((u32_t)rigel_rand32())

#define LWIP_PLATFORM_DIAG(x)    do { } while (0)
#define LWIP_PLATFORM_ASSERT(x)  do { kernel_puts("lwIP assert: " x "\r\n"); for (;;) ; } while (0)

#endif
