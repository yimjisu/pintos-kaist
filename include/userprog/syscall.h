#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

void syscall_init (void);

// 2-2
void check_address(uaddr);

//2-3
struct lock file_lock;
void halt (void);
void exit (int status);
bool create(const char *file, unsigned initial_size);
bool remove(const char *file);
int open(const char *file);
int filesize(int fd);
int read(int fd, void *buffer, unsigned size);
int write(int fd, const void *buffer, unsigned size);
void seek(int fd, unsigned position);
unsigned tell(int fd);
void close(int fd);
tid_t fork (const char *thread_name, struct intr_frame *f);
int exec (char *file_name);
static struct file* lookup_fd(int fd);
void remove_file(int fd);
// end 2-3

#endif /* userprog/syscall.h */
