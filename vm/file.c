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

/* The initializer of file vm */
void
vm_file_init (void) {
	struct file* file;
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
	
	file_page->lazy_aux = NULL;	
}

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

	pml4_set_dirty(thread_current() -> pml4, page->va, false);
	memset(page->va + page_read_bytes, 0, page_zero_bytes);
	return true;
}

/* Do the mmap */
void *
do_mmap (void *addr, size_t length, int writable,
		struct file *file, off_t offset) {
	struct file *new_file = file_reopen(file);
	void *addr_copy = addr;
	size_t read_bytes = length; // 오류나면 수정하기
	size_t zero_bytes = PGSIZE - read_bytes % PGSIZE;
	while (read_bytes > 0 || zero_bytes > 0) {
		/* Do calculate how to fill this page.
		 * We will read PAGE_READ_BYTES bytes from FILE
		 * and zero the final PAGE_ZERO_BYTES bytes. */
		size_t page_read_bytes = read_bytes < PGSIZE ? read_bytes : PGSIZE;
		size_t page_zero_bytes = PGSIZE - page_read_bytes;

		/* TODO: Set up aux to pass information to the lazy_load_file. */
		struct lazy_aux *aux = malloc(sizeof (struct lazy_aux));
		aux -> file = new_file;
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
	return addr_copy;
}

/* Do the munmap */
void
do_munmap (void *addr) {
	// unmaps the mapping for specified address range addr
	while(true) {
		struct page *page = spt_find_page(&thread_current() -> spt, addr);
		if(page == NULL) {
			return;
		}
		struct lazy_aux *aux = page->uninit.aux;

		uint64_t *pml4 = thread_current()->pml4;
		if(pml4_is_dirty(pml4, page->va)){
			file_write_at(aux->file, addr, aux->page_read_bytes, aux->ofs);
			pml4_set_dirty(thread_current() -> pml4, page->va, false);
		}
		pml4_clear_page(pml4, page->va);
		addr += PGSIZE;
	}
}
