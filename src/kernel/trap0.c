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
#include <sys/syscall.h>
#include <sys/scheduler.h>   /* OS_TaskExit(), current_task */

int trap0_handler(int num, long a1, long a2, long a3, long a4, long a5, long a6, long a7)
{
    (void)a4; (void)a5; (void)a6; (void)a7;

    switch (num) {

    /* ---- implementadas ---- */
    case SYS_EXIT:
        OS_TaskExit();            /* nao volta */
        return 0;
    case SYS_GETPID:
        return current_task ? (int)current_task->id : -1;
    case SYS_OPEN:
        return vfs_open((const char *)a1, (int)a2);
    case SYS_READ:
        return vfs_read((int)a1, (void *)a2, (uint32_t)a3);
    case SYS_WRITE:
        return vfs_write((int)a1, (void *)a2, (uint32_t)a3);
    case SYS_CLOSE:
        return vfs_close((int)a1);
    case SYS_IOCTL:
        return vfs_ioctl((int)a1, (int)a2, (void *)a3);
    case SYS_LSEEK:
        return vfs_lseek((int)a1, (uint32_t)a2, (int)a3);

    /* ---- TODO: ainda sem implementacao ---- */
    case SYS_SPAWN: return -1;   /* TODO */
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

/*

Sugestão para você

Uma biblioteca só, librigel.a, usada com -lrigel, como o -lc e o -lm. Dentro dela:

o glue da newlib (_exit, open, read, write, close, lseek, getpid, sbrk, isatty, fstat, kill);
os wrappers dos serviços do rigel (gotoxy, clrscr, fillrect, ppi_*, duart_*, rtc_*, eeprom_*, connect…).

E os headers organizados por serviço, no estilo:
<rigel/vga.h>, 
<rigel/ppi.h>, 
<rigel/duart.h>, 
<rigel/rtc.h>, 
<rigel/net.h>. 

Junto dela, dentro do mesmo “SDK” de apps, ficam o crt0 do app e o app.ld. 
Quem compila um app só precisa do -I do SDK e do -lrigel, e nunca vê os números das syscalls.

Se um dia quiser separar, o padrão da newlib seria libgloss-rigel.a só para o glue do C e librigel.a para 
os serviços. Mas enquanto for um projeto só seu, uma biblioteca é mais simples.

Como as 70 e tantas funções são o mesmo stub com números diferentes, o melhor é gerá-las a partir do 
syscall.h com um script, para os números nunca ficarem digitados em dois lugares. Posso escrever esse 
script e os headers, se você me disser dois pontos: o prefixo dos wrappers (gotoxy ou rigel_gotoxy?) e 
se os números dos SYS_* já estão estáveis, ou ainda vão mudar.

O que o arquivo faz:

Os case de OPEN, READ, WRITE, CLOSE, IOCTL e LSEEK são os do seu arquivo original, com as mesmas chamadas vfs_*.
Acrescentei SYS_EXIT, que chama OS_TaskExit(), e SYS_GETPID, que devolve current_task->id. Para isso o arquivo inclui <sys/scheduler.h>. Se current_task ou OS_TaskExit não estiverem declarados nesse header, o compilador vai reclamar e você me manda a mensagem de erro.
As outras 71 constantes devolvem -1 com  TODO , e o default também devolve -1.

A família escolhida: a nova (0x48 em diante) é a única que funciona, porque é a que tem vfs_* por trás. A família SYS_F* fica como -1 
TODO:. Assim cada operação tem só um número válido: open é 0x48, read é 0x4A, e assim por diante. A lib, o trap0.c e o syscall.h concordam.

Duas coisas ainda faltam, e nenhuma delas bloqueia o resto:

unlink, rename e mkdir da lib chamam syscalls que hoje são -1 TODO. Elas só passam a funcionar quando você escrever as funções correspondentes no VFS.
SYS_SETRAMVECTOR e SYS_READRTC também são -1 TODO.

*/