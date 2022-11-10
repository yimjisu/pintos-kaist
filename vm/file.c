/* file.c: Implementation of memory backed file object (mmaped object). */

#include "vm/vm.h"

#include "threads/vaddr.h" // P3-5
#include "userprog/process.h" // P3-5
#include "threads/mmu.h" // P3-5


static bool file_backed_swap_in (struct page *page, void *kva);
static bool file_backed_swap_out (struct page *page);
static void file_backed_destroy (struct page *page);

/* DO NOT MODIFY this struct */
static const struct page_operations file_ops = {
	.swap_in = file_backed_swap_in,
	.swap_out = file_backed_swap_out,
	.destroy = file_backed_destroy,
	.type = VM_FILE,
};


static struct list mmap_file_list;

struct mmap_file_info{
	struct list_elem elem;
	uint64_t start;
	uint64_t end;
};

/* The initializer of file vm */
void
vm_file_init (void) {
	struct file* file;
	list_init(&mmap_file_list);
}

/* Initialize the file backed page */
bool
file_backed_initializer (struct page *page, enum vm_type type, void *kva) {
	/* Set up the handler */
	page->operations = &file_ops;

	struct file_page *file_page = &page->file;
	file_page -> lazy_aux = (struct lazy_aux *)page->uninit.aux;
}

/* Swap in the page by read contents from the file. */
static bool
file_backed_swap_in (struct page *page, void *kva) {
	struct file_page *file_page UNUSED = &page->file;
	struct lazy_aux *aux = file_page -> lazy_aux;

	struct file *file = aux -> file;
	off_t ofs = aux ->ofs;
	size_t page_read_bytes = aux->page_read_bytes;
	size_t page_zero_bytes = aux->page_zero_bytes;

	file_seek(file, ofs);
	uint8_t *kpage = page->frame->kva;
	if(kpage == NULL)
		return false;
	
	if(file_read(file, kpage, page_read_bytes) != (off_t) page_read_bytes) {
		palloc_free_page(kpage);
		return false;
	}

	memset(page->va + page_read_bytes, 0, page_zero_bytes);
	return true;

}

/* Swap out the page by writeback contents to the file. */
static bool
file_backed_swap_out (struct page *page) {
	struct file_page *file_page UNUSED = &page->file;
	
	struct lazy_aux *aux = file_page -> lazy_aux;

	uint64_t *pml4 = thread_current()->pml4;
	if(pml4_is_dirty(pml4, page -> va)) {
		file_write_at(aux->file, page->va, aux->page_read_bytes, aux->ofs);
		pml4_set_dirty(pml4, page->va, false);
	}

	pml4_clear_page(pml4, page->va);
	page->frame = NULL;
	return true;
}

/* Destory the file backed page. PAGE will be freed by the caller. */
static void
file_backed_destroy (struct page *page) {
	struct file_page *file_page UNUSED = &page->file;
	// P3-5
	close(file_page -> lazy_aux -> file);
	file_page->lazy_aux = NULL;	
}
/* Do the mmap */

static bool
lazy_load_file (struct page *page, void *aux) {
	/* TODO: Load the segment from the file */
	/* TODO: This called when the first page fault occurs on address VA. */
	/* TODO: VA is available when calling this function. */
	
	struct lazy_aux* aux_info = (struct lazy_aux *) aux;
	struct file *file = aux_info -> file;
	off_t ofs = aux_info -> ofs;
	size_t page_read_bytes = aux_info -> page_read_bytes;
	size_t page_zero_bytes = aux_info -> page_zero_bytes;

	file_seek(file, ofs);
	uint8_t *kpage = page->frame->kva;
	if(kpage == NULL)
		return false;
	
	if(file_read(file, kpage, page_read_bytes) != (off_t) page_read_bytes) {
		palloc_free_page(kpage);
		return false;
	}

	memset(page->va + page_read_bytes, 0, page_zero_bytes);
	return true;
}

void *
do_mmap (void *addr, size_t length, int writable,
		struct file *file, off_t offset) {
	void *addr_copy = addr;
	size_t read_bytes = length <= file_length(file) ? length : file_length(file);
	size_t zero_bytes = PGSIZE - read_bytes % PGSIZE;
	while (read_bytes > 0 || zero_bytes > 0) {
		/* Do calculate how to fill this page.
		 * We will read PAGE_READ_BYTES bytes from FILE
		 * and zero the final PAGE_ZERO_BYTES bytes. */
		size_t page_read_bytes = read_bytes < PGSIZE ? read_bytes : PGSIZE;
		size_t page_zero_bytes = PGSIZE - page_read_bytes;

		/* TODO: Set up aux to pass information to the lazy_load_file. */
		struct lazy_aux *aux = malloc(sizeof (struct lazy_aux));
		aux -> file = file_reopen(file);
		aux -> ofs = offset;
		aux -> page_read_bytes = page_read_bytes;
		aux -> page_zero_bytes = page_zero_bytes;
		if (!vm_alloc_page_with_initializer (VM_ANON, addr,
					writable, lazy_load_file, (void *)aux)){
			return NULL;
		}

		/* Advance. */
		read_bytes -= page_read_bytes;
		zero_bytes -= page_zero_bytes;
		addr += PGSIZE;
		offset += page_read_bytes;
	}

	struct mmap_file_info* mfi = malloc (sizeof (struct mmap_file_info));
	mfi->start = (uint64_t) addr;
	mfi->end = (uint64_t) pg_round_down((uint64_t) addr + length -1);
	list_push_back(&mmap_file_list, &mfi->elem);

	return addr_copy;
}

/* Do the munmap */
void
do_munmap (void *addr) {
	if (list_empty (&mmap_file_list)) return;
	for (struct list_elem* i = list_front (&mmap_file_list); i != list_end (&mmap_file_list); i = list_next (i))
	{
		struct mmap_file_info* mfi = list_entry (i, struct mmap_file_info, elem);
		if (mfi -> start == (uint64_t) addr){
			for (uint64_t j = (uint64_t)addr; j<= mfi -> end; j += PGSIZE){
				struct page* page = spt_find_page(&thread_current() -> spt, (void*) j);
				spt_remove_page(&thread_current()->spt, page);
			}
			list_remove(&mfi->elem);
			free(mfi);
			return;
		}
	}
}
