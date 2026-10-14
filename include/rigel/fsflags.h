/*
 * rigel_fsflags.h -- constantes das syscalls de arquivo FAT (familia SYS_F*).
 *
 * ESTE E' O ARQUIVO MESTRE: fica em kernel/include/sys/rigel_fsflags.h.
 * O make da librigel copia para librigel/include/rigel/fsflags.h (nao edite la).
 */
#ifndef RIGEL_FSFLAGS_H
#define RIGEL_FSFLAGS_H

/* flags de rigel_ff_open (valores iguais aos FA_* do FatFS; combine com '|') */
#define RIGEL_FA_READ            0x01
#define RIGEL_FA_WRITE           0x02
#define RIGEL_FA_OPEN_EXISTING   0x00   /* abre; falha se nao existe          */
#define RIGEL_FA_CREATE_NEW      0x04   /* cria; falha se ja existe           */
#define RIGEL_FA_CREATE_ALWAYS   0x08   /* cria; se existe, zera              */
#define RIGEL_FA_OPEN_ALWAYS     0x10   /* abre; se nao existe, cria          */
#define RIGEL_FA_OPEN_APPEND     0x30   /* como OPEN_ALWAYS, posicao no fim   */

/* whence de rigel_ff_lseek */
#define RIGEL_SEEK_SET           0
#define RIGEL_SEEK_CUR           1
#define RIGEL_SEEK_END           2

/* comandos de rigel_ff_ioctl */
#define RIGEL_FIOC_TRUNCATE      1      /* corta o arquivo na posicao atual   */
#define RIGEL_FIOC_SYNC          2      /* grava buffers no cartao            */

#endif /* RIGEL_FSFLAGS_H */
