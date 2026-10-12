/*
 * elf_reloc.h -- relocacao na carga para ELFs ET_EXEC linkados em 0
 *
 * Substitui o patch da GOT e o apply_data_relocations do loader PIC.
 */
#ifndef ELF_RELOC_H
#define ELF_RELOC_H

#include <stdint.h>

#ifndef ELFREL_HOST_TEST
#include <fileio.h>              /* FIL, flseek, fread, FR_OK (ajuste ao seu projeto) */
#endif

#define ELFREL_OK            0
#define ELFREL_ERR_IO       -1   /* falha de leitura do arquivo */
#define ELFREL_ERR_RANGE    -2   /* r_offset fora da imagem carregada, ou impar */
#define ELFREL_ERR_UNSUPP   -3   /* tipo de relocacao que este loader nao trata */
#define ELFREL_ERR_NOSHDR   -4   /* ELF sem tabela de secoes (strip -s?) */

/*
 * Aplica as relocacoes R_68K_32 de todas as secoes .rela.* cujo alvo e'
 * alocavel (SHF_ALLOC), somando task_base ao valor ja gravado pelo linker.
 *
 *   fd          arquivo ELF aberto
 *   shoff       e_shoff       (offset da tabela de secoes)
 *   shnum       e_shnum
 *   shentsize   e_shentsize   (40 em ELF32)
 *   task_base   endereco onde a imagem (vaddr 0) foi carregada
 *   image_size  maior (p_vaddr + p_memsz) dos PT_LOAD: limite para os patches
 *   applied     (opcional) recebe quantas relocacoes foram aplicadas
 *
 * Deve ser chamada DEPOIS de carregar todos os segmentos PT_LOAD.
 * Retorna ELFREL_OK ou um codigo ELFREL_ERR_*.
 */
int elf_apply_relocs(FIL *fd, uint32_t shoff, uint16_t shnum, uint16_t shentsize,
                     uint32_t task_base, uint32_t image_size, uint32_t *applied);

#endif
