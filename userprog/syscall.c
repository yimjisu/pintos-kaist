#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/loader.h"
#include "userprog/gdt.h"
#include "threads/flags.h"
#include "intrinsic.h"
#include "user/syscall.h"

void syscall_entry (void);
void syscall_handler (struct intr_frame *);

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
	printf ("system call!\n");
	switch(f->R.rax) { 
		case SYS_HALT:
			halt();
			break;
		case SYS_EXIT:
			exec(f->R.rdi);
			break;
		case SYS_FORK:
			f->R.rax = fork(f->R.rdi, f);
			break;
		case SYS_EXEC:
			if(exec(f->R.rdi)==-1) exit(-1);
			break;
		case SYS_WAIT:
			f->R.rax = wait(f->R.rdi);
			break;
	}
	thread_exit ();
}

void halt (void) {
	power_off();
}

void exit(int status) {
	thread_current() -> exit_status = status;
	thread_exit();
}

pid_t fork(const char *thread_name, struct intr_frame* f) {
	thread_current()->parent_if = f; // ?? memcpy?
	return process_fork(thread_name, f);
}

int exec(const char *cmd_line) {
	check_address(cmd_line);
	// Never returns if successful
	// Otherwise process terminates with exit state -1

	// file descriptors remoain open => Make Copy
	// char *fn_copy = palloc_get_page(PAL_ZERO);
	// if(fn_copy == NULL) exit(-1);
	// strlcpy(fn_copy, cmd_line,  strlen(cmd_line) + 1);

	if (process_exec(cmd_line) == -1) return -1;
	return 0;
}

int wait(pid_t pid) {
	return process_wait(pid);
}

void check_address(void *addr){
	// Check if user-provided pointer address is valid.
	// Kernel must access memory through pointers provided by a user program.
	// 1. Null Pointer
	// 2. Unmapped virtual memory
	// 3. Pointer to kernel virtual address space (KERN_BASE)
	// Terminate the user process

	// First Method : verify validity of user-provided pointer & dereference
	if (addr == NULL) exit(-1);
	if (is_kernel_vaddr(addr)) exit(-1);
	if (pml4_get_page(thread_current()->pml4, addr) == NULL) exit(-1);
}