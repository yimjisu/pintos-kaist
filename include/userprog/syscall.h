#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

typedef int pid_t;

void syscall_init (void);
// 2-3 start
struct lock *filesys_lock;
// 2-3 end

#endif /* userprog/syscall.h */
