#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/loader.h"
#include "userprog/gdt.h"
#include "threads/flags.h"
#include "intrinsic.h"

#include "filesys/file.h" //P2-3
#include "filesys/filesys.h" //P2-3
#include "threads/palloc.h" //P2-3
#include "vm/file.h" //P3-5
#include "filesys/inode.h" //P4-2
#include "filesys/directory.h" //P4-2
#include "filesys/fat.h" //P4-2

void syscall_entry (void);
void syscall_handler (struct intr_frame *);

void check_address(const uint64_t *uaddr); //P2-2


// start P2-3 
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
int add_file(struct file *file);
void remove_file(int fd);
// end P2-3

// start P3-5
void *mmap (void *addr, size_t length, int writable, int fd, off_t offset);
void munmap (void *addr);
// end P3-5

//P4-2 start
bool chdir(const char *dir);
bool mkdir(const char *dir);
bool readdir(int fd, char *dir);
bool isdir(int fd);
int inumber(int fd);
int symlink (const char *target, const char *link);
//P4-2 end

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
	lock_init(&file_lock); //P2-3
}

/* The main system call interface */
void
syscall_handler (struct intr_frame *f UNUSED) {
	// TODO: Your implementation goes here.
	// %rax : syscall num
	// arg 순서 : %rdi, %rsi, %rdx, %r10, %r8, %r9
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
			if (exec(f->R.rdi) == -1) {
				exit(-1);
			}
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
		case SYS_DUP2:
			f->R.rax = dup2(f->R.rdi, f->R.rsi);
			break;
		case SYS_MMAP:
			f->R.rax = mmap((void *)f->R.rdi, (size_t)f->R.rsi, (int)f->R.rdx, (int)f->R.r10, (off_t)f->R.r8);
			break;
		case SYS_MUNMAP:
			munmap(f->R.rdi);
			break;
		//P4-2 start
		case SYS_CHDIR:
			f->R.rax = chdir(f->R.rdi);
			break;
		case SYS_MKDIR:
			f->R.rax = mkdir(f->R.rdi);
			break;
		case SYS_READDIR:
			f->R.rax = readdir(f->R.rdi, f->R.rsi);
			break;
		case SYS_ISDIR:
			f->R.rax =  isdir(f->R.rdi);
			break;
		case SYS_INUMBER:
			f->R.rax = inumber(f->R.rdi);
			break;
		case SYS_SYMLINK:
			f->R.rax = symlink(f->R.rdi, f->R.rsi);
			break;
		//P4-2 end
		default:
			exit(-1);
			break;
	}
	// thread_exit ();
}

// start P2-3
void halt (void) {
	power_off();
}

void exit(int status) {
	thread_current()->exit_status = status;
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
	if (fn_copy == NULL) {
		exit(-1);
	}
	strlcpy(fn_copy, cmd_line, strlen(cmd_line) + 1);

	if (process_exec(fn_copy) == -1) {
		return -1;
	}
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
	int fd = add_file(open);
	
	if (fd == -1) {
		file_close(open);
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
	check_valid_buffer(buffer, size);
	
	int read;
	struct file *open = lookup_fd(fd);
	if (open == NULL) {
		return -1;
	}

	struct thread *curr = thread_current();
	if (open == 1) { //stdin
		if(curr->stdin_num == 0) { // stdin is closed. do nothing.
			remove_file(fd);
			return -1;
		}
		*(char *)buffer = input_getc();
		read = size;
	}
	else if (open == 2) { //stdout
		return -1;
	}
	else {
		lock_acquire(&file_lock);
		if (!inode_isdir(open->inode)) read = file_read(open, buffer, size);//P4-2
		else read = -1;
		lock_release(&file_lock);
	}
	return read;
}

int write (int fd, const void *buffer, unsigned size) {
	check_address(buffer);
	int write;
	struct file *open = lookup_fd(fd);
	struct thread *curr = thread_current();

	if (open == NULL) {
		return -1;
	}
	if (open == 1) { //stdin
		return -1;
	}else if (open == 2) { //stdout
		if(curr->stdout_num == 0) { // stdout is closed. do nothing
			return -1;
		}
		putbuf(buffer, size);
		write = size;
	}
	else {
		lock_acquire(&file_lock);
		if (!inode_isdir(open->inode)) write = file_write(open, buffer, size);//P4-2
		else write = -1;
		lock_release(&file_lock);
	}
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

	struct thread *curr = thread_current();
	if(open == 1) {
		curr->stdin_num --;
	}else if(open == 2) {
		curr->stdout_num --;
	}
	
	remove_file(fd);

	if (open <= 2) {
		return;
	}

	if(open->dup_num == 0){
		file_close(open);
	}else{
		open->dup_num --;
	}
}

static struct file *lookup_fd(int fd) {
	struct thread *curr = thread_current();
	if (fd < 0 || fd >= FDCOUNT_LIMIT) {
		return NULL;
	}
	return curr->files[fd];
}

int add_file(struct file *file)
{
	struct thread *curr = thread_current();
	struct file **fdt = curr->files;

	while (curr->fd_index < FDCOUNT_LIMIT && fdt[curr->fd_index]) {
		curr->fd_index = curr->fd_index + 1;
	}

	if (curr->fd_index >= FDCOUNT_LIMIT) {
		return -1;
	}

	fdt[curr->fd_index] = file;
	return curr->fd_index;
}

void remove_file(int fd) {
	struct thread *cur = thread_current();
	if (fd < 0 || fd >= FDCOUNT_LIMIT) {
		return;
	}
	cur->files[fd] = NULL;
}

// end 2-3

// start 2-extra
int dup2(int oldfd, int newfd) {
	struct file *open = lookup_fd(oldfd);
	if (open == NULL) {
		return -1;
	}
	if (oldfd == newfd) {
		return newfd;
	}

	struct thread *cur = thread_current();	
	if (open == 1) { // stdin
		cur->stdin_num++;
	}
	else if (open == 2) { // stdout
		cur->stdout_num++;
	}
	else {
		open->dup_num++;
	}

	close(newfd);

	cur->files[newfd] = open;
	return newfd;
}

// start P3-5
void *mmap (void *addr, size_t length, int writable, int fd, off_t offset) {
	if (pg_ofs(addr) != 0
	|| length == 0
	|| offset > PGSIZE) {
		return NULL;
	}

	for (int i = 0; i <= length; i++) {
		if(addr+i == NULL|| is_kernel_vaddr(addr+i)) {
			return NULL;
		}
	}
	for (int i = 0; i <= length; i+= PGSIZE) {
		if (spt_find_page (&thread_current() -> spt, addr + i) != NULL) {
			return NULL;
		}
	}

	struct file *open = lookup_fd(fd);
	if(open == NULL || open == 1 || open == 2) {
		return NULL;
	}
	return do_mmap(addr, length, writable, open, offset);
}

void munmap (void *addr) {
	do_munmap(addr);
}
// end P3-5

//P4-2 start
bool chdir(const char *dir) {
	lock_acquire(&file_lock);
	bool res = filesys_chdir(dir);
	lock_release(&file_lock);
    return res;
}

bool mkdir(const char *dir) {
	lock_acquire(&file_lock);
    bool res = filesys_create_dir(dir);
    lock_release(&file_lock);
    return res;
}

bool readdir(int fd, char *name) {
	lock_acquire(&file_lock);
	struct file *open = lookup_fd(fd);
	if (open == NULL) return false;
	struct inode *inode = file_get_inode(open);
	if (inode == NULL) return false;
	if (!inode_isdir(inode)) return false;

	struct dir *dir = open->dir;
    // if (dir->pos == 0) {
    //     dir_seek(dir, 2 * sizeof(struct dir_entry));
	// }
	bool ret = dir_readdir(dir, name);
	lock_release(&file_lock);
	return ret;
}

bool isdir(int fd) {
	lock_acquire(&file_lock);
	struct file *open = lookup_fd(fd);
	if (open==NULL) return false;
	bool res = inode_isdir(file_get_inode(open));
	lock_release(&file_lock);
    return res;
}

int inumber(int fd) {
	lock_acquire(&file_lock);
	struct file *open = lookup_fd(fd);
	if (open==NULL) return false;
	bool res = inode_get_inumber(file_get_inode(open));
	lock_release(&file_lock);
    return res;
}

int symlink (const char *target, const char *link) {
	lock_acquire(&file_lock);
    bool success = false;
	char* link_copy = (char *)malloc(strlen(link) + 1);
    strlcpy(link_copy, link, strlen(link) + 1); 

    char* link_file = (char *)malloc(strlen(link_copy) + 1);
    struct dir* dir = parse_path(link_copy, link_file);

    cluster_t inode_cluster = fat_create_chain(0);

    success = (dir != NULL
               && link_inode_create(inode_cluster, target)
               && dir_add(dir, link_file, cluster_to_sector(inode_cluster), 0));

    if (!success && inode_cluster != 0) {
        fat_remove_chain(inode_cluster, 0);
	}
    
    dir_close(dir);
	lock_release(&file_lock);
    return success - 1;
}

//P4-2 end



// start 2-2
void check_address (const uint64_t *addr) {
	struct thread *curr = thread_current();
	if (addr == NULL || !(is_user_vaddr(addr))) { // || pml4_walk(curr->pml4, addr, 0) == NULL) {
		exit(-1);
	}
}
// end 2-2

void check_valid_buffer(void *buffer, unsigned size) {
	struct page *page = spt_find_page(&thread_current() -> spt, buffer);
	if(page != NULL && page->writable == false)
		exit(-1);
}