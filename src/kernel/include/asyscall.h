#ifndef __APP_SYSCALL_H__
#define __APP_SYSCALL_H__
#include <unistd.h>
#include <rigel_types.h>


//VFS
extern int open(const char *path, int flags,...);
//extern int read(int fd, void *buf, int count);
_READ_WRITE_RETURN_TYPE read (int __fd, void *__buf, size_t __nbyte);
//extern int write(int fd, const void *buf, int count);
_READ_WRITE_RETURN_TYPE write (int __fd, const void *__buf, size_t __nbyte);
extern int close(int fd);
extern int ioctl(int fd, int cmd, void *arg);
//extern int lseek(int fd, uint32_t offset, int whence);
off_t   lseek (int __fildes, off_t __offset, int __whence);

//Video text functions
extern int video_writechar(int fd, char ch);
extern int gotoxy(int fd, uint16_t x,uint16_t y);
extern int gohome(int fd);
extern int setcolor(int fd, uint8_t txtcolor,uint8_t bgcolor);
extern int clrscr(int fd);

//Video graphic functions
int drawVLine(int fd, uint16_t x, uint16_t y, uint16_t h, char color);
int drawHLine(int fd, int x, int y, int w, char color);
int drawLine(int fd, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, char color);
int drawCircle(int fd, uint16_t x0, uint16_t y0, uint16_t r, char color);
int drawCircleHelper(int fd, uint16_t x0, uint16_t y0, uint16_t r, unsigned char cornername, char color);
int fillCircle(int fd, uint16_t x0, uint16_t y0, uint16_t r, char color);
int fillCircleHelper(int fd, uint16_t x0, uint16_t y0, uint16_t r, unsigned char cornername, uint16_t delta, char color);
int drawRoundRect(int fd, uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t r, char color);
int fillRoundRect(int fd, uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t r, char color);
int fillRect(int fd, uint16_t x, uint16_t y, uint16_t w, uint16_t h, char color);

//File system calls
int fsync(int fd ) ;
int funlink(int fd ) ;
int flseek(int fd , FSIZE_t offset) ;

//File administration tem na newlib
//int ftell(int fd ) ;
//int fsize(int fd ) ;
//int feof(int fd ) ;

//System adminstration
int frename(int fd, char* path_old, const char* path_new) ;
//int fstat(int fd, char* path, FILINFO* fno) ;
//int fopendir(int fd, DIR* dp, char* path) ;
//int freaddir(int fd, DIR* dp, FILINFO* fno) ;
//int fclosedir(int fd, DIR* dp) ;
int fmkdir(int fd, char* path) ;


//File internals
//int findfirst(DIR* dp, FILINFO* fno, char* path, char* pattern) ;
//int findnext(DIR* dp, FILINFO* fno) ;

//RTC Functions
void rtc_read_time(rtc_time_t *rrtc) ;
void rtc_write_time(rtc_time_t *wrtc) ;

//Special input
char getkbdkey(void);

#endif //#indef __APP_SYSCALL_H__
