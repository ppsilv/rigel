/*
 * trap0.c -- despachante de syscalls do kernel Rigel (TRAP #0)
 *
 * SUBSTITUI o trap0.c inteiro. Todas as constantes de sys/syscall.h
 * aparecem aqui. As que ainda nao tem implementacao devolvem -1.
 *
 * Familia de arquivos usada pelos apps: SYS_OPEN/READ/WRITE/CLOSE/IOCTL/LSEEK
 * (0x48..0x4E). A familia SYS_F* (4..0x1A) fica reservada, devolvendo -1.
 */
#include <stdint.h>
#include <sys/vfs.h>
#include <sys/ksyscall.h>
#include <sys/scheduler.h>   /* OS_TaskExit(), current_task */

/*
 * Depuracao do SYS_WRITE: imprime na tela do KERNEL (VGA, via kernel_puts)
 * os argumentos recebidos e o retorno do vfs_write.
 * Para ligar: descomente a linha "#define TRAP_DEBUG 1".
 */
/* #define TRAP_DEBUG 1 */
#ifdef TRAP_DEBUG
extern void kernel_puts(const char *s);
extern void kernel_puthex(uint32_t v);
#define DBG_S(s) kernel_puts(s)
#define DBG_H(v) kernel_puthex((uint32_t)(v))
#else
#define DBG_S(s) ((void)0)
#define DBG_H(v) ((void)0)
#endif

/* DIAGNOSTICO de spawn/exit/waitpid: escreve direto no VGA do kernel.
 * Depois de resolver, comente a linha abaixo. */
#define TRAP_DIAG 1
#ifdef TRAP_DIAG
extern void kernel_puts(const char *s);
extern void kernel_puthex(uint32_t v);
#endif

extern int sys_spawn(const char *path, char **argv);   /* spawn.c */
extern int OS_TaskAlive(int id);                       /* scheduler.c */

int trap0_handler(int num, long a1, long a2, long a3, long a4, long a5, long a6, long a7)
{
    (void)a4; (void)a5; (void)a6; (void)a7;

    switch (num) {

    /* ---- implementadas ---- */
    case SYS_EXIT:
#ifdef TRAP_DIAG
        kernel_puts("\n[kernel] SYS_EXIT pid=");
        kernel_puthex(current_task ? (uint32_t)current_task->id : 0);
        kernel_puts("\n");
#endif
        OS_TaskExit();            /* nao volta */
        return 0;
    case SYS_GETPID:
        return current_task ? (int)current_task->id : -1;
    case SYS_OPEN:
        return vfs_open((const char *)a1, (int)a2);
    case SYS_READ:
        return vfs_read((int)a1, (void *)a2, (uint32_t)a3);
    case SYS_WRITE: {
        int r;
        DBG_S("\n[W fd="); DBG_H(a1);
        DBG_S(" buf=");     DBG_H(a2);
        DBG_S(" len=");     DBG_H(a3);
        DBG_S("]\n");
        r = vfs_write((int)a1, (void *)a2, (uint32_t)a3);
        DBG_S("[W ret=");   DBG_H(r);
        DBG_S("]\n");
        return r;
    }
    case SYS_CLOSE:
        return vfs_close((int)a1);
    case SYS_IOCTL:
        return vfs_ioctl((int)a1, (int)a2, (void *)a3);
    case SYS_LSEEK:
        return vfs_lseek((int)a1, (uint32_t)a2, (int)a3);

    case SYS_SPAWN:
        return sys_spawn((const char *)a1, (char **)a2);
    case SYS_WAITPID:
#ifdef TRAP_DIAG
        kernel_puts("\n[kernel] WAITPID pid="); kernel_puthex((uint32_t)a1); kernel_puts("\n");
#endif
        while (OS_TaskAlive((int)a1));    /* o tick preempta; o filho continua rodando */
#ifdef TRAP_DIAG
        kernel_puts("\n[kernel] WAITPID terminou\n");
#endif
        return 0;

    /* ---- TODO: ainda sem implementacao ---- */
    case SYS_SLEEP: return -1;   /* TODO */
    case SYS_YIELD: return -1;   /* TODO */
    case SYS_FOPEN: return -1;   /* TODO */
    case SYS_FCLOSE: return -1;   /* TODO */
    case SYS_FREAD: return -1;   /* TODO */
    case SYS_FWRITE: return -1;   /* TODO */
    case SYS_FSYNC: return -1;   /* TODO */
    case SYS_FLSEEK: return -1;   /* TODO */
    case SYS_FIOCTL: return -1;   /* TODO */
    case SYS_DUP: return -1;   /* TODO */
    case SYS_FUNLINK: return -1;   /* TODO */
    case SYS_CHMOD: return -1;   /* TODO */
    case SYS_FTELL: return -1;   /* TODO */
    case SYS_FSIZE: return -1;   /* TODO */
    case SYS_FEOF: return -1;   /* TODO */
    case SYS_FRENAME: return -1;   /* TODO */
    case SYS_FSTAT: return -1;   /* TODO */
    case SYS_FOPENDIR: return -1;   /* TODO */
    case SYS_FREADDIR: return -1;   /* TODO */
    case SYS_FMOUNT: return -1;   /* TODO */
    case SYS_FCLOSEDIR: return -1;   /* TODO */
    case SYS_FMKDIR: return -1;   /* TODO */
    case SYS_FUNMOUNT: return -1;   /* TODO */
    case SYS_FFINDFIRST: return -1;   /* TODO */
    case SYS_FFINDNEXT: return -1;   /* TODO */
    case SYS_VGAPUTCHAR: return -1;   /* TODO */
    case SYS_GOTOXY: return -1;   /* TODO */
    case SYS_GOHOME: return -1;   /* TODO */
    case SYS_SETTEXTCOLOR: return -1;   /* TODO */
    case SYS_CLRSCR: return -1;   /* TODO */
    case SYS_DRAWVLINE: return -1;   /* TODO */
    case SYS_DRAWHLINE: return -1;   /* TODO */
    case SYS_DRAWLINE: return -1;   /* TODO */
    case SYS_DRAWCIRCLE: return -1;   /* TODO */
    case SYS_DRAWCIRCLEHELPER: return -1;   /* TODO */
    case SYS_FILLCIRCLE: return -1;   /* TODO */
    case SYS_FILLCIRCLEHELPER: return -1;   /* TODO */
    case SYS_DRAWROUNDRECT: return -1;   /* TODO */
    case SYS_FILLROUNDRECT: return -1;   /* TODO */
    case SYS_FILLRECT: return -1;   /* TODO */
    case SYS_READTIME: return -1;   /* TODO */
    case SYS_WRITETIME: return -1;   /* TODO */
    case SYS_EEPROMREADBYTE: return -1;   /* TODO */
    case SYS_EEPROMWRITEBYTE: return -1;   /* TODO */
    case SYS_RINGBUFFERREAD: return -1;   /* TODO */
    case SYS_RINGBUFFERWRITE: return -1;   /* TODO */
    case SYS_DUARTACFG: return -1;   /* TODO */
    case SYS_DUARTAREAD: return -1;   /* TODO */
    case SYS_DUARTAWRITE: return -1;   /* TODO */
    case SYS_DUARTBCFG: return -1;   /* TODO */
    case SYS_DUARTBREAD: return -1;   /* TODO */
    case SYS_DUARTBWRITE: return -1;   /* TODO */
    case SYS_PPIACFG: return -1;   /* TODO */
    case SYS_PPIPORTAREAD: return -1;   /* TODO */
    case SYS_PPIPORTAWRITE: return -1;   /* TODO */
    case SYS_PPIBCFG: return -1;   /* TODO */
    case SYS_PPIPORTBREAD: return -1;   /* TODO */
    case SYS_PPIPORTBWRITE: return -1;   /* TODO */
    case SYS_PPICCFG: return -1;   /* TODO */
    case SYS_PPIPORTCREAD: return -1;   /* TODO */
    case SYS_PPIPORTCWRITE: return -1;   /* TODO */
    case SYS_CONNECT: return -1;   /* TODO */
    case SYS_BIND: return -1;   /* TODO */
    case SYS_LISTEN: return -1;   /* TODO */
    case SYS_ACCEPT: return -1;   /* TODO */
    case SYS_SETSOCKOPT: return -1;   /* TODO */
    case SYS_SETRAMVECTOR: return -1;   /* TODO */
    case SYS_READRTC: return -1;   /* TODO */
    case SYS_WRITERTC: return -1;   /* TODO */
    case SYS_SINC: return -1;   /* TODO */

    default:
        return -1;                /* numero de syscall desconhecido */
    }
}