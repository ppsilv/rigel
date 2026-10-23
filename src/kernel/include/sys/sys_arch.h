#ifndef __SYS_ARCH_H__
#define  __SYS_ARCH_H__

int ff_mutex_create(int vol);
void ff_mutex_delete(int vol);
int ff_mutex_take(int vol);
void ff_mutex_give(int vol);

#endif