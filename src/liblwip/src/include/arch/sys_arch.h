/*
 * arch/sys_arch.h -- tipos do lwIP para o kernel Rigel (NO_SYS = 0).
 * Coloque em: <pasta do lwip port>/include/arch/sys_arch.h
 * (o lwIP faz #include "arch/sys_arch.h" a partir de lwip/sys.h)
 */
#ifndef RIGEL_SYS_ARCH_H
#define RIGEL_SYS_ARCH_H

#include <stdint.h>

#define SYS_MBOX_NULL   ((sys_mbox_t)0)
#define SYS_SEM_NULL    ((sys_sem_t)0)

typedef struct rigel_sem   *sys_sem_t;
typedef struct rigel_sem   *sys_mutex_t;     /* mutex = semaforo binario */
typedef struct rigel_mbox  *sys_mbox_t;
typedef int                 sys_thread_t;    /* id da task */

/* se SYS_LIGHTWEIGHT_PROT == 1 no lwipopts.h */
typedef uint32_t            sys_prot_t;


/* o lwIP 2.x precisa destas validacoes (ponteiro nulo = invalido) */
#define sys_sem_valid(s)           (((s) != NULL) && (*(s) != NULL))
#define sys_sem_set_invalid(s)     do { if ((s) != NULL) { *(s) = NULL; } } while (0)
#define sys_mutex_valid(m)         (((m) != NULL) && (*(m) != NULL))
#define sys_mutex_set_invalid(m)   do { if ((m) != NULL) { *(m) = NULL; } } while (0)
//#define sys_mbox_valid(m)          (((m) != NULL) && (*(m) != NULL))
#define sys_mbox_set_invalid(m)    do { if ((m) != NULL) { *(m) = NULL; } } while (0)


int sys_mbox_valid(struct rigel_mbox **m);

#endif
