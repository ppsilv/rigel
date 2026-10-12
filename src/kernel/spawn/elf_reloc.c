/*
 * elf_reloc.c -- relocacao na carga para o loader de ELF do Orion
 *
 * COMO COMPILAR OS PROGRAMAS (sem -fPIC, sem -msep-data, libc normal):
 *
 *     CFLAGS  = -m68000 -O2 -ffreestanding ...
 *     LDFLAGS = -T prog.ld -Wl,-q          # -q = --emit-relocs
 *
 *   - o prog.ld continua linkando em 0 (ORIGIN do slot = 0)
 *   - NAO use "strip -s"/"strip --strip-all": isso remove as relocacoes.
 *     Para reduzir o arquivo use:  m68k-elf-strip --strip-debug prog.elf
 *   - confira com:  m68k-elf-readelf -r prog.elf
 *     (so devem aparecer R_68K_32 e, no maximo, R_68K_PC8/PC16/PC32)
 *
 * COMO FUNCIONA:
 *   O linker ja gravou na imagem o valor "simbolo + addend" calculado
 *   como se o programa rodasse em 0. Para cada R_68K_32 basta somar
 *   task_base ao longo que esta' em task_base + r_offset.
 *   Relocacoes PC-relativas continuam certas quando o simbolo esta' dentro
 *   do mesmo slot, entao nao precisam de ajuste.
 *
 * Detalhes importantes:
 *   - com --emit-relocs o r_offset dos ET_EXEC e' um VADDR (nao um offset de
 *     secao), por isso o endereco do patch e' task_base + r_offset;
 *   - com -g tambem existem .rela.debug_*, cujo r_offset NAO e' endereco de
 *     memoria. Por isso so' sao tratadas as .rela cujo alvo (sh_info) tem
 *     SHF_ALLOC;
 *   - as secoes sao procuradas por TIPO (SHT_RELA), nao por nome: nao
 *     precisa ler .shstrtab nem .symtab.
 *   - Limite: uma relocacao contra simbolo ABSOLUTO (ex.: um endereco de
 *     hardware definido com .equ/PROVIDE) tambem seria somada com task_base.
 *     Use #define (constante, sem relocacao) para enderecos de hardware.
 */
#include "stdio.h"
#include "elf_reloc.h"

#ifndef ELFREL_PTR
#define ELFREL_PTR(a) ((uint8_t *)(a))
#endif

#ifndef ELFREL_LOG
#include <stdio.h>
#define ELFREL_LOG(...) kprintf(__VA_ARGS__)
#endif

#define SHT_RELA      4
#define SHF_ALLOC     0x2
#define SHDR_SIZE     40
#define RELA_SIZE     12
#define RELA_CHUNK    16          /* relocacoes lidas por fread */

#define R_68K_NONE    0
#define R_68K_32      1
#define R_68K_PC32    4
#define R_68K_PC16    5
#define R_68K_PC8     6

static uint32_t be32(const uint8_t *p)
{
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8)  |  (uint32_t)p[3];
}

static void put_be32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)(v >> 24);
    p[1] = (uint8_t)(v >> 16);
    p[2] = (uint8_t)(v >> 8);
    p[3] = (uint8_t)v;
}

static int read_at(FIL *fd, uint32_t off, void *buf, unsigned int n)
{
    unsigned int got = 0;

    f_lseek(fd, off);
    if (f_read(fd, buf, n, &got) != FR_OK || got != n)
        return -1;
    return 0;
}

int elf_apply_relocs(FIL *fd, uint32_t shoff, uint16_t shnum, uint16_t shentsize,
                     uint32_t task_base, uint32_t image_size, uint32_t *applied)
{
    uint8_t  sh[SHDR_SIZE];
    uint8_t  tgt[SHDR_SIZE];
    uint8_t  buf[RELA_SIZE * RELA_CHUNK];
    uint32_t total = 0;

    if (applied)
        *applied = 0;

    if (shoff == 0 || shnum == 0 || shentsize < SHDR_SIZE) {
        ELFREL_LOG("elf_reloc: ELF sem tabela de secoes.\n");
        return ELFREL_ERR_NOSHDR;
    }

    for (uint16_t i = 0; i < shnum; i++) {
        if (read_at(fd, shoff + (uint32_t)i * shentsize, sh, SHDR_SIZE))
            return ELFREL_ERR_IO;

        if (be32(sh + 4) != SHT_RELA)
            continue;

        uint32_t rel_off  = be32(sh + 16);
        uint32_t rel_size = be32(sh + 20);
        uint32_t target   = be32(sh + 28);       /* sh_info: secao relocada */

        if (target >= shnum)
            continue;
        if (read_at(fd, shoff + target * (uint32_t)shentsize, tgt, SHDR_SIZE))
            return ELFREL_ERR_IO;
        if (!(be32(tgt + 8) & SHF_ALLOC))        /* ex.: .rela.debug_info */
            continue;

        uint32_t n    = rel_size / RELA_SIZE;
        uint32_t done = 0;

        while (done < n) {
            uint32_t chunk = n - done;
            if (chunk > RELA_CHUNK)
                chunk = RELA_CHUNK;

            if (read_at(fd, rel_off + done * RELA_SIZE, buf, chunk * RELA_SIZE))
                return ELFREL_ERR_IO;

            for (uint32_t k = 0; k < chunk; k++) {
                const uint8_t *e = buf + k * RELA_SIZE;
                uint32_t r_offset = be32(e);
                uint32_t type     = be32(e + 4) & 0xFF;

                switch (type) {
                case R_68K_NONE:
                case R_68K_PC32:
                case R_68K_PC16:
                case R_68K_PC8:
                    break;                        /* nada a fazer */

                case R_68K_32:
                    if (r_offset > image_size || image_size - r_offset < 4 ||
                        (r_offset & 1)) {
                        ELFREL_LOG("elf_reloc: r_offset 0x%lx fora da imagem "
                                   "(0x%lx bytes) ou impar.\n",
                                   (unsigned long)r_offset,
                                   (unsigned long)image_size);
                        return ELFREL_ERR_RANGE;
                    }
                    {
                        uint8_t *p = ELFREL_PTR(task_base + r_offset);
                        put_be32(p, be32(p) + task_base);
                    }
                    total++;
                    break;

                default:
                    ELFREL_LOG("elf_reloc: tipo %lu nao suportado em 0x%lx.\n",
                               (unsigned long)type, (unsigned long)r_offset);
                    return ELFREL_ERR_UNSUPP;
                }
            }
            done += chunk;
        }
    }

    if (applied)
        *applied = total;
    return ELFREL_OK;
}
