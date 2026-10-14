/*
 * rigel_dirent.h -- entrada de diretorio devolvida por SYS_FREADDIR.
 *
 * ESTE E' O ARQUIVO MESTRE: fica em kernel/include/sys/rigel_dirent.h.
 * O make da librigel copia para librigel/include/rigel/dirent.h (nao edite la).
 * Kernel e apps usam o mesmo layout.
 */
#ifndef RIGEL_DIRENT_H
#define RIGEL_DIRENT_H

#include <stdint.h>

#define RIGEL_NAME_MAX   63          /* nome maior que isso e' truncado */

/* bits de 'attr' (iguais aos do FatFS) */
#define RIGEL_ATTR_RDO   0x01
#define RIGEL_ATTR_HID   0x02
#define RIGEL_ATTR_SYS   0x04
#define RIGEL_ATTR_DIR   0x10
#define RIGEL_ATTR_ARC   0x20

typedef struct {
    uint32_t size;                   /* tamanho em bytes (0 para diretorio) */
    uint16_t date;                   /* data FAT: ((ano-1980)<<9)|(mes<<5)|dia */
    uint16_t time;                   /* hora FAT: (hora<<11)|(min<<5)|(seg/2) */
    uint8_t  attr;                   /* RIGEL_ATTR_* */
    char     name[RIGEL_NAME_MAX + 1];
} rigel_dirent_t;

#endif /* RIGEL_DIRENT_H */
