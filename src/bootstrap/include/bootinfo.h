#ifndef BOOTINFO_H
#define BOOTINFO_H
#include <stdint.h>

#define BOOTINFO_MAGIC  0x52474C31UL      /* 'RGL1' */
#define BOOTINFO_ADDR   0x00081F00UL      /* logo abaixo do kernel */

#define KERNEL_PATH    "/boot/kernel.bin"
#define KERNEL_BASE    0x00082000UL
#define KERNEL_LIMIT   0x000A2000UL
#define KERNEL_MAGIC   0x4B524E4CUL      /* 'KRNL' */
#define CHUNK          4096

enum { BK_OK = 0, BK_OPEN, BK_EMPTY, BK_TOOBIG, BK_READ, BK_MAGIC, BK_END };

struct bootinfo {
    uint32_t magic;
    uint16_t version;       /* 1 */
    uint16_t size;          /* sizeof(struct bootinfo) */
    uint32_t ram_base;
    uint32_t ram_top;
    uint32_t duart_base;
    uint32_t video_base;    /* 0 se nao ha */
    uint32_t boot_drive;
    uint32_t kernel_base;
    uint32_t kernel_size;   /* bytes lidos do arquivo */
    uint32_t kernel_end;    /* _end do kernel, incluindo .bss */
    uint32_t jump_table;    /* tabela de saltos do Rigel, 0 se nao ha */
};
#endif