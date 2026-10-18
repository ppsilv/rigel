#ifndef __TRAP0_H__
#define __TRAP0_H__

#include <stdint.h>
#include <sys/rigel_dirent.h>
#include <sys/scheduler.h>

extern long sys_opendir(const char *path);              /* fsdir.c */
extern long sys_readdir(long h, rigel_dirent_t *ent);
extern long sys_closedir(long h);
extern long sys_findfirst(const char *path, const char *pattern, rigel_dirent_t *ent);  /* sys_fsdir.c */
extern long sys_findnext(long h, rigel_dirent_t *ent);
extern void sys_dir_task_exit(int owner);

/* sys_fsfile.c: arquivos FAT (familia SYS_F*) */
extern long sys_fopen(const char *path, long flags);
extern long sys_fclose(long h);
extern long sys_fread(long h, void *buf, unsigned long size);
extern long sys_fwrite(long h, const void *buf, unsigned long size);
extern long sys_fsync(long h);
extern long sys_flseek(long h, long off, long whence);
extern long sys_fioctl(long h, long cmd, void *arg);
extern long sys_ftell(long h);
extern long sys_fsize(long h);
extern long sys_feof(long h);
extern long sys_funlink(const char *path);
extern long sys_frename(const char *oldp, const char *newp);
extern long sys_fstat(const char *path, rigel_dirent_t *ent);
extern long sys_fmkdir(const char *path);
extern long sys_chmod(const char *path, long attrs);

/* sys_proc.c */
extern long sys_sleep(long ms);
extern long sys_yield(void);
extern long sys_fmount(const char *path);
extern long sys_funmount(const char *path);
extern void sys_fs_task_exit(int owner);

extern int sys_spawn(const char *path, char **argv);   /* spawn.c */
extern int OS_TaskAlive(int id);                       /* scheduler.c */
extern void OS_SleepTicks(uint32_t ticks);
extern uint8_t sys_getkey();
extern uint8_t sys_haskey();

extern uint32_t day_uptime;
extern void sys_setramvector(long stub_addr, long handler_addr);

/* sys_fsrtc.c: RTC DS1307 via Pico */
extern long sys_readrtc(void);
extern long sys_writertc(long rtc);

/*VGA*/
extern long sys_vga_putchar(long c);
extern long sys_gotoxy(long x, long y);
extern long sys_gohome(void);
extern long sys_settextcolor(long a1, long a2);
extern long sys_clrscr(void);
extern long sys_drawvline(long x, long y, long h, long color);
extern long sys_drawhline(long x, long y, long w, long color);
extern long sys_drawline(long x0, long y0, long x1, long y1, long color);
extern long sys_drawcircle(long x0, long y0, long r, long color);
extern long sys_drawcirclehelper(long x0, long y0, long r, long cornername, long color);
extern long sys_fillcircle(long x0, long y0, long r, long color);
extern long sys_fillcirclehelper(long x0, long y0, long r, long cornername, long delta, long color);
extern long sys_drawroundrect(long x, long y, long w, long h, long r, long color);
extern long sys_fillroundrect(long x, long y, long w, long h, long r, long color);
extern long sys_fillrect(long x, long y, long w, long h, long color);
extern long sys_chdir(const char *path);
extern long sys_getcwd(char *buf, long size);

extern long sys_socket(long d, long t, long p);
extern long sys_bind(long fd, const void *a, long len);
extern long sys_listen(long fd, long backlog);
extern long sys_accept(long fd, void *a, long *len);
extern long sys_connect(long fd, const void *a, long len);
extern long sys_setsockopt(long fd, long level, long opt, const void *val, long len);
extern void sys_net_task_exit(int owner);

#endif