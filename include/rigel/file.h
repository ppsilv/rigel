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
long rigel_opendir(char *path, void *dir);
long rigel_readdir(void *dir, void *file_info);
long rigel_mount(char *path);
long rigel_closedir(void *dir);
long rigel_unmount(char *path);
long rigel_findfirst(void *dir, void *file_info, char *path, char *pattern);
long rigel_findnext(void *dir, void *file_info);

#ifdef __cplusplus
}
#endif

#endif /* RIGEL_FILE_H */
