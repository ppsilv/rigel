/*
 * loader_elf.c -- SUBSTITUI o loader_pic.c inteiro.
 *
 * Carrega um ELF ET_EXEC linkado em 0 (compilado com -Wl,-q) num slot,
 * relocaciona (elf_reloc.c) e cria a task. Sem GOT, sem A5, sem PIC.
 */
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <fileio.h>
#include <sys/slots.h>
#include <sys/elf.h>
#include <sys/scheduler.h>
#include "elf_reloc.h"

#define TASK_STACK_SIZE  8192
/* SLOT_SIZE vem de <sys/slots.h> */
#define IMAGE_LIMIT      (SLOT_SIZE - TASK_STACK_SIZE)   /* imagem nao pode invadir a pilha */

#define BE16(p,o) ((uint16_t)(((p)[o] << 8) | (p)[(o)+1]))
#define BE32(p,o) (((uint32_t)(p)[o] << 24) | ((uint32_t)(p)[(o)+1] << 16) | \
                   ((uint32_t)(p)[(o)+2] << 8) | (uint32_t)(p)[(o)+3])

/* Retorna 0 em sucesso (task criada e na fila), -1 em erro. */
int load_elf_task(FIL *fd, int argc, char *argv[])
{
    uint8_t  hb[52];
    elf32_program_header ph;
    unsigned int n;
    uint32_t image_size = 0, applied = 0;
    int nload = 0;

    OS_Reap();                       /* recolhe tasks que terminaram: devolve slots e TCBs */

    int slot = Slots_Alloc();
    if (slot < 0) { kprintf("Sem slots livres.\n"); return -1; }
    uint32_t task_base = Slots_BaseAddr(slot);

    f_lseek(fd, 0);
    if (f_read(fd, hb, sizeof hb, &n) != FR_OK || n != sizeof hb) {
        kprintf("Nao leu o header do ELF.\n"); goto fail;
    }
    if (hb[0] != 0x7F || hb[1] != 'E' || hb[2] != 'L' || hb[3] != 'F' || hb[6] != 1) {
        kprintf("Header ELF invalido.\n"); goto fail;
    }
    if (hb[4] != ID_32BIT || hb[5] != ID_BIG_ENDIAN) {
        kprintf("Nao e' ELF 32-bit big-endian.\n"); goto fail;
    }
    if (BE16(hb,0x10) != ET_EXEC || BE16(hb,0x12) != EM_68K) {
        kprintf("ELF nao e' executavel 68k.\n"); goto fail;
    }

    uint32_t entry     = BE32(hb,0x18);
    uint32_t phoff     = BE32(hb,0x1C);
    uint32_t shoff     = BE32(hb,0x20);
    uint16_t phentsize = BE16(hb,0x2A);
    uint16_t phnum     = BE16(hb,0x2C);
    uint16_t shentsize = BE16(hb,0x2E);
    uint16_t shnum     = BE16(hb,0x30);

    for (uint16_t i = 0; i < phnum; i++) {
        f_lseek(fd, phoff + (uint32_t)i * phentsize);
        if (f_read(fd, &ph, sizeof ph, &n) != FR_OK || n != sizeof ph) {
            kprintf("Nao leu program header.\n"); goto fail;
        }
        if (ph.type == PT_DYNAMIC || ph.type == PT_SHLIB || ph.type == PT_INTERP) {
            kprintf("ELF dinamico: nao suportado.\n"); goto fail;
        }
        if (ph.type != PT_LOAD || ph.memsz == 0)
            continue;

        /* programa linkado em 0: paddr == vaddr */
        if (ph.filesz > ph.memsz || ph.paddr > IMAGE_LIMIT ||
            ph.memsz > IMAGE_LIMIT - ph.paddr) {
            kprintf("Segmento PT_LOAD nao cabe no slot.\n"); goto fail;
        }
        uint8_t *dst = (uint8_t *)(task_base + ph.paddr);

        f_lseek(fd, ph.offset);
        if (f_read(fd, dst, ph.filesz, &n) != FR_OK || n != ph.filesz) {
            kprintf("Falha lendo PT_LOAD.\n"); goto fail;
        }
        if (ph.memsz > ph.filesz)
            memset(dst + ph.filesz, 0, ph.memsz - ph.filesz);

        if (ph.paddr + ph.memsz > image_size)
            image_size = ph.paddr + ph.memsz;
        nload++;
    }

    if (nload == 0 || entry >= image_size) {
        kprintf("ELF sem segmento carregavel / entry invalido.\n"); goto fail;
    }

    int rc = elf_apply_relocs(fd, shoff, shnum, shentsize, task_base, image_size, &applied);
    if (rc != ELFREL_OK) {
        kprintf("Relocacao falhou (rc=%d).\n", rc); goto fail;
    }

    /* Todo app real tem relocacoes (ate' o crt0 tem jsr/lea absolutos).
     * applied==0 quer dizer que o ELF foi linkado SEM -Wl,-q: rodar assim
     * pula para enderecos baixos (ROM) em vez do slot. */
    if (applied == 0) {
        kprintf("ELF sem relocacoes: linke com -Wl,-q (nao use strip -s).\n");
        goto fail;
    }

    //kprintf("slot=%02d base=0x%08lx size=0x%lx relocs=%lu entry=0x%08lx\n",
    //        slot, (unsigned long)task_base, (unsigned long)image_size,
    //        (unsigned long)applied, (unsigned long)(task_base + entry));

    {
        void (*entry_fn)(void) = (void (*)(void))(task_base + entry);
        uint8_t *task_stack = (uint8_t *)(task_base + SLOT_SIZE - TASK_STACK_SIZE);

        /* id = OS_SLOT_ID_BASE + slot: e' assim que o OS_Reap sabe qual slot liberar */
        tcb_t *tcb = OS_TaskCreate(entry_fn, NULL, task_stack, TASK_STACK_SIZE,
                                   OS_SLOT_ID_BASE + slot);
        if (!tcb) { kprintf("Falha ao criar task.\n"); goto fail; }
        return 0;   /* o slot agora pertence a' task */
    }

fail:
    Slots_Free(slot);
    return -1;
}