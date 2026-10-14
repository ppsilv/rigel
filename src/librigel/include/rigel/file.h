/* GERADO por tools/gen_librigel.py -- NAO EDITE. */
#ifndef RIGEL_FILE_H
#define RIGEL_FILE_H

#ifdef __cplusplus
extern "C" {
#endif

long rigel_fsync(long fd);
long rigel_ioctl(long fd, long cmd, void *arg);
long rigel_dup(long fd_old, long fd_new);
long rigel_chmod(char *path, long attrs);
long rigel_tell(long fd);
long rigel_size(long fd);
long rigel_eof(long fd);
long rigel_stat(char *path, void *stat_buf);
long rigel_opendir(char *path);
long rigel_readdir(long dir, void *ent);
long rigel_mount(char *path);
long rigel_closedir(long dir);
long rigel_unmount(char *path);
long rigel_findfirst(char *path, char *pattern, void *ent);
long rigel_findnext(long dir, void *ent);
long rigel_ff_open(char *path, long flags);
long rigel_ff_close(long h);
long rigel_ff_read(long h, void *buf, long size);
long rigel_ff_write(long h, void *buf, long size);
long rigel_ff_sync(long h);
long rigel_ff_lseek(long h, long offset, long whence);
long rigel_ff_ioctl(long h, long cmd, void *arg);

#ifdef __cplusplus
}
#endif

#endif /* RIGEL_FILE_H */
