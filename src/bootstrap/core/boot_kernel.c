#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "ff.h"                          /* FS: ou o seu fileio.h */
#include "bootinfo.h"




static struct bootinfo * const bi = (struct bootinfo *)BOOTINFO_ADDR;

int boot_load_kernel(const char *path)
{
    FIL      f;
    UINT     got;
    uint32_t size, done = 0;
    uint8_t *dst = (uint8_t *)KERNEL_BASE;

    FRESULT r = f_open(&f, path, FA_READ);
    if (r != FR_OK) {
        if( r == 5 ){
            printf("File or directory does not exist...\n");
        }
        printf("kernel: nao abriu %s (FRESULT %d)\n", path, (int)r);
        return BK_OPEN;
    }    
    size = (uint32_t)f_size(&f);                         /* FS */

    if (size < 16) { f_close(&f); printf("kernel: arquivo vazio\n"); return BK_EMPTY; }
    if (size > KERNEL_LIMIT - KERNEL_BASE) {
        f_close(&f); printf("kernel: grande demais (%lu)\n", (unsigned long)size);
        return BK_TOOBIG;
    }

    printf("kernel: %lu bytes -> 0x%lx ", (unsigned long)size, KERNEL_BASE);
    while (done < size) {
        UINT n = (size - done > CHUNK) ? CHUNK : (UINT)(size - done);
        if (f_read(&f, dst + done, n, &got) != FR_OK || got != n) {   /* FS */
            f_close(&f); printf("\nkernel: erro de leitura em %lu\n", (unsigned long)done);
            return BK_READ;
        }
        done += got;
        putchar('.');
    }
    f_close(&f);                                         /* FS */
    putchar('\n');

    /* cabecalho da imagem: [0] bra.w _start  [4] magic  [8] _end do kernel */
//    uint32_t magic = *(volatile uint32_t *)(KERNEL_BASE + 4);
    uint32_t kend  = *(volatile uint32_t *)(KERNEL_BASE + 8);

    //if (magic != KERNEL_MAGIC) {
    //    printf("kernel: magic invalido (0x%lx)\n", (unsigned long)magic);
    //    return BK_MAGIC;
    //}
    //if (kend < KERNEL_BASE + size || kend > KERNEL_LIMIT) {
    //    printf("kernel: _end invalido (0x%lx)\n", (unsigned long)kend);
    //    return BK_END;
    //}

    memset(bi, 0, sizeof *bi);
    bi->magic       = BOOTINFO_MAGIC;
    bi->version     = 1;
    bi->size        = sizeof *bi;
    bi->ram_base    = 0x00080000UL;
    bi->ram_top     = 0x000FFFF0UL;
    bi->duart_base  = 0x00FF9000UL;
    bi->kernel_base = KERNEL_BASE;
    bi->kernel_size = size;
    bi->kernel_end  = kend;
    return BK_OK;
}

void boot_jump(void) __attribute__((noreturn));
void boot_jump(void)
{
    __asm__ volatile (
        "move.w  #0x2700,%%sr   \n\t"    /* IRQ mascaradas */
        "move.l  %0,%%a0        \n\t"    /* A0 = &bootinfo */
        "jmp     %c1"                    /* header do kernel: bra.w _start */
        : : "a"(bi), "i"(KERNEL_BASE));
    __builtin_unreachable();
}