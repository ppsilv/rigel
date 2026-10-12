#include <stdint.h>

// Definição do tipo para as chamadas de sistema
typedef long (*sys_call_t)(void);

/*
Alguns pontos que valem revisão sua, não mudei nada sozinho porque são decisão de design:
sys_sinc — supus que seja "sync" (fsync, força gravação no cartão/HD), só recebendo o fd.
    Confirma se é isso mesmo ou se tem outro parâmetro.
sys_opendir/sys_findfirst — segui o padrão do FatFS (DIR, FILINFO como ponteiros opacos 
    passados por fora, igual f_opendir/f_findfirst da lib) em vez de retornar um handle. Se 
    seu VFS abstrai isso diferente (tipo um fd de diretório igual arquivo normal), a assinatura 
    muda bastante.
sys_mount/sys_unmount — coloquei recebendo path, mas se for por número de unidade 
    (tipo FatFS f_mount com índice 0/1 pra ATA/SD) é long drive em vez de char *path.
sys_accept — addrlen como long * (ponteiro) porque no padrão BSD o kernel escreve de 
    volta o tamanho real do endereço preenchido; se seu socket vai ser mais simples que isso, 
    dá pra tirar esse ponteiro.
*/
/* processo / task */
long sys_exit(long code);
long sys_spawn(char *path);
long sys_sleep(long ticks);
long sys_yield(void);

/* arquivo / vfs */
long sys_open(char *path, long flags);
long sys_close(long fd);
long sys_read(long fd, char *buf, long len);
long sys_write(long fd, char *buf, long len);
long sys_sinc(long fd);
long sys_lseek(long fd, long offset, long whence);
long sys_ioctl(long fd, long cmd, void *arg);
long sys_dup(long fd_old, long fd_new);
long sys_unlink(char *path);
long sys_chmod(char *path, long attrs);
long sys_tell(long fd);
long sys_size(long fd);
long sys_eof(long fd);
long sys_rename(char *old_path, char *new_path);
long sys_stat(char *path, void *stat_buf);
long sys_opendir(char *path, void *dir);
long sys_readdir(void *dir, void *file_info);
long sys_mount(char *path);
long sys_closedir(void *dir);
long sys_fmkdir(char *path);
long sys_unmount(char *path);
long sys_findfirst(void *dir, void *file_info, char *path, char *pattern);
long sys_findnext(void *dir, void *file_info);

/* picovga */
long sys_vga_putchar(long c);
long sys_gotoxy(long x, long y);
long sys_gohome(void);
long sys_settextcolor(long color);
long sys_clrscr(void);
long sys_drawvline(long x, long y, long h, long color);
long sys_drawhline(long x, long y, long w, long color);
long sys_drawline(long x0, long y0, long x1, long y1, long color);
long sys_drawcircle(long x0, long y0, long r, long color);
long sys_drawcirclehelper(long x0, long y0, long r, long cornername, long color);
long sys_fillcircle(long x0, long y0, long r, long color);
long sys_fillcirclehelper(long x0, long y0, long r, long cornername, long delta, long color);
long sys_drawroundrect(long x, long y, long w, long h, long r, long color);
long sys_fillroundrect(long x, long y, long w, long h, long r, long color);
long sys_fillrect(long x, long y, long w, long h, long color);

/* rtc */
long sys_readtime(void *tm_out);
long sys_writetime(void *tm_in);

/* eeprom */
long sys_eepromreadbyte(long addr);
long sys_eepromwritebyte(long addr, long value);

/* ring buffer */
long sys_ringbufferread(void *rb);
long sys_ringbufferwrite(void *rb, long value);

/* duart */
long sys_duartacfg(long baud, long params);
long sys_duartaread(void);
long sys_duartawrite(long value);
long sys_duartbcfg(long baud, long params);
long sys_duartbread(void);
long sys_duartbwrite(long value);

/* ppi */
long sys_ppiacfg(long mode);
long sys_ppiportaread(void);
long sys_ppiportawrite(long value);
long sys_ppibcfg(long mode);
long sys_ppiportbread(void);
long sys_ppiportbwrite(long value);
long sys_ppiccfg(long mode);
long sys_ppiportcread(void);
long sys_ppiportcwrite(long value);

/* sockets (w5100 / lwip) */
long sys_connect(long sockfd, void *addr, long addrlen);
long sys_bind(long sockfd, void *addr, long addrlen);
long sys_listen(long sockfd, long backlog);
long sys_accept(long sockfd, void *addr, long *addrlen);
long sys_setsockopt(long sockfd, long level, long optname, void *optval, long optlen);
long sys_getpid(void);
long sys_setramvector(long stub_addr, long handler_addr);

// Tabela de despacho direta (índice = número da syscall)
const sys_call_t sys_call_table[] = {
    (sys_call_t) sys_exit,                  /*0*/
    (sys_call_t) sys_spawn,                 /*1*/
    (sys_call_t) sys_sleep,                 /*2*/
    (sys_call_t) sys_yield,                 /*3*/
    (sys_call_t) sys_open,                  /*4*/
    (sys_call_t) sys_close,                 /*5*/
    (sys_call_t) sys_read,                  /*6*/
    (sys_call_t) sys_write,                 /*7*/
    (sys_call_t) sys_sinc,                  /*8*/
    (sys_call_t) sys_lseek,                 /*9*/
    (sys_call_t) sys_ioctl,                 /*10*/
    (sys_call_t) sys_dup,                   /*11*/
    (sys_call_t) sys_unlink,                /*12*/
    (sys_call_t) sys_chmod,                 /*13*/
    (sys_call_t) sys_tell,                  /*14*/
    (sys_call_t) sys_size,                  /*15*/
    (sys_call_t) sys_eof,                   /*16*/
    (sys_call_t) sys_rename,                /*17*/
    (sys_call_t) sys_stat,                  /*18*/
    (sys_call_t) sys_opendir,               /*19*/
    (sys_call_t) sys_readdir,               /*20*/
    (sys_call_t) sys_mount,                 /*21*/
    (sys_call_t) sys_closedir,              /*22*/
    (sys_call_t) sys_fmkdir,                /*23*/
    (sys_call_t) sys_unmount,               /*24*/
    (sys_call_t) sys_findfirst,             /*25*/
    (sys_call_t) sys_findnext,              /*26*/
    (sys_call_t) sys_vga_putchar,           /*27*/
    (sys_call_t) sys_gotoxy,                /*28*/
    (sys_call_t) sys_gohome,                /*29*/
    (sys_call_t) sys_settextcolor,          /*30*/
    (sys_call_t) sys_clrscr,                /*31*/
    (sys_call_t) sys_drawvline,             /*32*/
    (sys_call_t) sys_drawhline,             /*33*/
    (sys_call_t) sys_drawline,              /*34*/
    (sys_call_t) sys_drawcircle,            /*35*/
    (sys_call_t) sys_drawcirclehelper,      /*36*/
    (sys_call_t) sys_fillcircle,            /*37*/
    (sys_call_t) sys_fillcirclehelper,      /*38*/
    (sys_call_t) sys_drawroundrect,         /*39*/
    (sys_call_t) sys_fillroundrect,         /*40*/
    (sys_call_t) sys_fillrect,              /*41*/
    (sys_call_t) sys_readtime,              /*42*/
    (sys_call_t) sys_writetime,             /*43*/
    (sys_call_t) sys_eepromreadbyte,        /*44*/
    (sys_call_t) sys_eepromwritebyte,       /*45*/
    (sys_call_t) sys_ringbufferread,        /*46*/
    (sys_call_t) sys_ringbufferwrite,       /*47*/
    (sys_call_t) sys_duartacfg,             /*48*/
    (sys_call_t) sys_duartaread,            /*49*/
    (sys_call_t) sys_duartawrite,           /*50*/
    (sys_call_t) sys_duartbcfg,             /*51*/
    (sys_call_t) sys_duartbread,            /*52*/
    (sys_call_t) sys_duartbwrite,           /*53*/
    (sys_call_t) sys_ppiacfg,               /*54*/
    (sys_call_t) sys_ppiportaread,          /*55*/
    (sys_call_t) sys_ppiportawrite,         /*56*/
    (sys_call_t) sys_ppibcfg,               /*57*/
    (sys_call_t) sys_ppiportbread,          /*58*/
    (sys_call_t) sys_ppiportbwrite,         /*59*/
    (sys_call_t) sys_ppiccfg,               /*60*/
    (sys_call_t) sys_ppiportcread,          /*61*/
    (sys_call_t) sys_ppiportcwrite,         /*62*/
    (sys_call_t) sys_connect,               /*63*/
    (sys_call_t) sys_bind,                  /*64*/
    (sys_call_t) sys_listen,                /*65*/
    (sys_call_t) sys_accept,                /*66*/
    (sys_call_t) sys_setsockopt,            /*67*/
    (sys_call_t) sys_getpid,                /*68*/
    (sys_call_t) sys_setramvector,          /*69*/
};
/*
Reparei que SYS_CONNECT/SYS_BIND/SYS_LISTEN/SYS_ACCEPT/SYS_SETSOCKOPT 
estão na lista — bate com o que você mencionou de querer lwIP sobre o 
driver do W5100 mais pra frente. Se quiser, quando for implementar 
essa parte eu ajudo a desenhar como esses sockets se encaixam no 
open/read/write do VFS (tipo BSD sockets sobre file descriptor) ou 
se prefere uma trap separada mesmo.
*/
#define NUM_SYSCALLS (sizeof(sys_call_table) / sizeof(sys_call_t))

// Função principal chamada pelo tratador de exceção do 68k
long kernel_syscall_dispatcher(long trap_num, long *stack_args) {
    if (trap_num < 0 || trap_num >= NUM_SYSCALLS) {
        // Tratar erro de syscall inválida
        return -1;
    }
    // Pega o ponteiro da função direto na tabela
    sys_call_t func = sys_call_table[trap_num];
    if (!func) {
        return -1;
    }
    // Executa diretamente. A própria assinatura da função em C
    // dita quantos argumentos ela consome da pilha/ponteiro.
    return func();
}

