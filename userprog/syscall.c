#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/loader.h"
#include "userprog/gdt.h"
#include "threads/flags.h"
#include "intrinsic.h"

void syscall_entry (void);
void syscall_handler (struct intr_frame *);

// 2-2
void check_address(uaddr);

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

// 2-3
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

	while (cur->fd < FDCOUNT_LIMIT && files[cur->fd]) {
		cur->fd = cur->fd + 1;
	}

	if (cur->fd >= FDCOUNT_LIMIT) {
		fd = -1;
		file_close(open);
	}

	else {
		files[cur->fd] = file;
		fd = cur->fd;
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

	if (fd == 0) {
		*(char *)buffer = input_getc(); //stdin
		read = size;
	}
	else {
		open = lookup_fd(fd);
		if (open == NULL) {
			return -1;
		}
		else {
			lock_acquire(&filesys_lock);
			read = file_read(open, buffer, size);
			lock_release(&filesys_lock);
		}
	}
	return read;
}

int write (int fd, const void *buffer, unsigned length) {
	check_address(buffer);
	int write;
	struct file *open;
	lock_acquire(&filesys_lock);
	if (fd == 1) {
		putbuf(buffer, size); //stdout
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
	lock_release(&filesys_lock);
	return write;
}

void seek (int fd, unsigned position) {
	struct file *open = lookup_fd(fd);
	if (open <= 2) {// 초기값 2로 설정. 0: 표준 입력, 1: 표준 출력
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

void remove_file(int fd)
{
	struct thread *cur = thread_current();
	if (fd < 0 || fd >= FDCOUNT_LIMIT)
		return;
	cur->files[fd] = NULL;
}



// void halt (void) NO_RETURN;
// void exit (int status) NO_RETURN;
// pid_t fork (const char *thread_name);
// int exec (const char *file);
// int wait (pid_t);
// bool create (const char *file, unsigned initial_size);
// bool remove (const char *file);
// int open (const char *file);
// int filesize (int fd);
// int read (int fd, void *buffer, unsigned length);
// int write (int fd, const void *buffer, unsigned length);
// void seek (int fd, unsigned position);
// unsigned tell (int fd);
// void close (int fd);