#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/loader.h"
#include "userprog/gdt.h"
#include "threads/flags.h"
#include "intrinsic.h"

#include "filesys/file.h"
#include "threads/palloc.h"

void syscall_entry (void);
void syscall_handler (struct intr_frame *);

// 2-2
void check_address(const uint64_t *uaddr);

//2-3 
struct lock file_lock;
void halt (void);
void exit (int status);
tid_t fork (const char *thread_name);
int exec (const char *cmd_line);
bool create(const char *file, unsigned initial_size);
bool remove(const char *file);
int open(const char *file);
int filesize(int fd);
int read(int fd, void *buffer, unsigned size);
int write(int fd, const void *buffer, unsigned size);
void seek(int fd, unsigned position);
unsigned tell(int fd);
void close(int fd);


static struct file* lookup_fd(int fd);
void remove_file(int fd);
// end 2-3

/* System call.
 *
 * Previously system call services was handled by the interrupt handler
 * (e.g. int 0x80 in linux). However, in x86-64, the manufacturer supplies
 * efficient path for requesting the system call, the `syscall` instruction.
 *
 * The syscall instruction works by reading the values from the the Model
 * Specific Register (MSR). For the details, see the manual. */

#define MSR_STAR 0xc0000081         /* Segment selector msr */
#define MSR_LSTAR 0xc0000082        /* Long mode SYSCALL target */
#define MSR_SYSCALL_MASK 0xc0000084 /* Mask for the eflags */

void
syscall_init (void) {
	write_msr(MSR_STAR, ((uint64_t)SEL_UCSEG - 0x10) << 48  |
			((uint64_t)SEL_KCSEG) << 32);
	write_msr(MSR_LSTAR, (uint64_t) syscall_entry);

	/* The interrupt service rountine should not serve any interrupts
	 * until the syscall_entry swaps the userland stack to the kernel
	 * mode stack. Therefore, we masked the FLAG_FL. */
	write_msr(MSR_SYSCALL_MASK,
			FLAG_IF | FLAG_TF | FLAG_DF | FLAG_IOPL | FLAG_AC | FLAG_NT);
	lock_init(&file_lock);
}

/* The main system call interface */
void
syscall_handler (struct intr_frame *f UNUSED) {
	// TODO: Your implementation goes here.
	// %rax : syscall num
	// arg 순서 : %rdi, %rsi, %rdx, %r10, %r8, %r9
	
	switch (f->R.rax) {
		case SYS_CREATE:
			// bool create(const char *file, unsigned initial_size)
			// 
			f->R.rax = create(f->R.rdi, f->R.rsi);
	}

	printf ("system call!\n");
	switch(f->R.rax) { 
		case SYS_HALT:
			halt();
			break;
		case SYS_EXIT:
			exit(f->R.rdi);
			break;
		case SYS_FORK:
			memcpy(&thread_current()->parent_if, f, sizeof(struct intr_frame)); // child를 만드는 데 intr_frame 정보가 필요하기 때문
			f->R.rax = fork(f->R.rdi);
			break;
		case SYS_EXEC:
			if(exec(f->R.rdi)==-1) exit(-1);
			break;
		case SYS_WAIT:
			f->R.rax = process_wait(f->R.rdi);
			break;
		case SYS_CREATE:
		f->R.rax = create(f->R.rdi, f->R.rsi);
		break;
		case SYS_REMOVE:
			f->R.rax = remove(f->R.rdi);
			break;
		case SYS_OPEN:
			f->R.rax = open(f->R.rdi);
			break;
		case SYS_FILESIZE:
			f->R.rax = filesize(f->R.rdi);
			break;
		case SYS_READ:
			f->R.rax = read(f->R.rdi, f->R.rsi, f->R.rdx);
			break;
		case SYS_WRITE:
			f->R.rax = write(f->R.rdi, f->R.rsi, f->R.rdx);
			break;
		case SYS_SEEK:
			seek(f->R.rdi, f->R.rsi);
			break;
		case SYS_TELL:
			f->R.rax = tell(f->R.rdi);
			break;
		case SYS_CLOSE:
			close(f->R.rdi);
			break;
	}
	thread_exit ();
}

// start 2-2
void check_address (const uint64_t *uaddr) {
	struct thread *cur = thread_current();
	if (uaddr == NULL || !(is_user_vaddr(uaddr)) || pml4_get_page(cur->pml4, uaddr) == NULL) {
		exit(-1);
	}
}
// end 2-2

// start 2-3
void halt (void) {
	power_off();
}

void exit(int status) {
	thread_current() -> exit_status = status;
	printf("%s: exit(%d)\n", thread_name(), status); // 2-4
	thread_exit();
}

tid_t fork(const char *thread_name) {
	struct intr_frame *if_ = &thread_current()->parent_if;
	return process_fork(thread_name, if_);
}

int exec(const char *cmd_line) {
	check_address(cmd_line);

	char *fn_copy = palloc_get_page(PAL_ZERO);
	if(fn_copy == NULL) exit(-1);
	strlcpy(fn_copy, cmd_line,  strlen(cmd_line) + 1);

	if (process_exec(fn_copy) == -1) return -1;
	return 0;
}

bool create(const char *file, unsigned initial_size) {
	check_address(file);
	return filesys_create(file, initial_size);
}

bool remove (const char *file) {
	check_address(file);
	return filesys_remove(file);
}

int open (const char *file) {
	check_address(file);
	struct file *open = filesys_open(file);
	if (open == NULL) {
		return -1;
	}

	int fd;
	
	struct thread *cur = thread_current();
	struct file **files = cur->files;

	while (cur->fd_index < FDCOUNT_LIMIT && files[cur->fd_index]) {
		cur->fd_index = cur->fd_index + 1;
	}

	if (cur->fd_index >= FDCOUNT_LIMIT) {
		fd = -1;
		file_close(open);
	}

	else {
		files[cur->fd_index] = file;
		fd = cur->fd_index;
	}

	return fd;
}

int filesize (int fd) {
	struct file *open = lookup_fd(fd);
	if (open == NULL) {
		return -1;
	}
	return file_length(open);
}

int read(int fd, void *buffer, unsigned size) {
	check_address(buffer);
	int read;
	struct thread *cur = thread_current();
	struct file *open;

	if (fd == 0) { //stdin
		*(char *)buffer = input_getc();
		read = size;
	}
	else if (fd == 1) { //stdout
		return -1;
	}
	else {
		open = lookup_fd(fd);
		if (open == NULL) {
			return -1;
		}
		else {
			lock_acquire(&file_lock);
			read = file_read(open, buffer, size);
			lock_release(&file_lock);
		}
	}
	return read;
}

int write (int fd, const void *buffer, unsigned size) {
	check_address(buffer);
	int write;
	struct file *open;
	lock_acquire(&file_lock);
	if (fd == 0) { //stdin
		return -1;
	}
	if (fd == 1) { //stdout
		putbuf(buffer, size);
		write = size;
	}
	else {
		open = lookup_fd(fd);
		if (open == NULL) {
			write = -1;
		}
		else {
			write = file_write(open, buffer, size);
		}
	}
	lock_release(&file_lock);
	return write;
}

void seek (int fd, unsigned position) {
	struct file *open = lookup_fd(fd);
	if (open <= 2) {
		return;
	}
	open->pos = position;
}

unsigned tell (int fd) {
	struct file *open= lookup_fd(fd);
	if (open <= 2) {
		return;
	}
	return file_tell(open);
}

void close (int fd) {
	struct file *open = lookup_fd(fd);
	if (open == NULL) {
		return;
	}
	remove_file(fd);
}

static struct file *lookup_fd(int fd) {
	struct thread *cur = thread_current();
	if (fd < 0 || fd >= FDCOUNT_LIMIT) {
		return NULL;
	}
	return cur->files[fd];
}

void remove_file(int fd) {
	struct thread *cur = thread_current();
	if (fd < 0 || fd >= FDCOUNT_LIMIT)
		return;
	cur->files[fd] = NULL;
}

// end 2-3