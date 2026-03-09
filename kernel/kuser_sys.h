#ifndef	_USER_SYS_H
#define	_USER_SYS_H

/*
 * Functions used by kuser tasks,
 * i.e. tasks running at EL0 but compiled & linked as part of the kernel code.
 * cf kuser_sys.S
 */

struct stat; 

// syscalls
int call_sys_write(int fd, char * buf, int n);  // needed by user_donut
int call_sys_read(int, void*, int);
int call_sys_open(const char * buf, int omode);
int call_sys_mknod(const char * buf, short major, short minor);
int call_sys_mkdir(const char * buf);
int call_sys_dup(int fd);
int call_sys_fstat(int fd, struct stat*);
int call_sys_close(int);
void *call_sys_sbrk(int increment);  // needed by user_donut
int call_sys_sleep(int ms); // needed by user_donut
int call_sys_fork();
int call_sys_exit(int);
int call_sys_exec(char *, char **);

void user_flush_dcache_range(void* start_addr, void* end_addr);

#endif  /*_USER_SYS_H */
