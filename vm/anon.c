/* anon.c: Implementation of page for non-disk image (a.k.a. anonymous page). */

#include "vm/vm.h"
#include "devices/disk.h"
#include "bitmap.h" // P3-5
#include "threads/vaddr.h" // P3-5
#include <bitmap.h>
#include "threads/mmu.h"

/* DO NOT MODIFY BELOW LINE */
static struct disk *swap_disk;
static bool anon_swap_in (struct page *page, void *kva);
static bool anon_swap_out (struct page *page);
static void anon_destroy (struct page *page);

struct bitmap *swap_slot; //P3-5
const size_t SECTORS_PER_PAGE = PGSIZE / DISK_SECTOR_SIZE;

/* DO NOT MODIFY this struct */
static const struct page_operations anon_ops = {
	.swap_in = anon_swap_in,
	.swap_out = anon_swap_out,
	.destroy = anon_destroy,
	.type = VM_ANON,
};

/* Initialize the data for anonymous pages */
void
vm_anon_init (void) {
	/* TODO: Set up the swap_disk. */
	swap_disk = NULL;
	// P3-5
	swap_disk = disk_get(1, 1); // SWAP
	int bit_len = disk_size(swap_disk) / SECTORS_PER_PAGE; 
	swap_slot = bitmap_create(bit_len);
}

/* Initialize the file mapping */
bool
anon_initializer (struct page *page, enum vm_type type, void *kva) {
	/* Set up the handler */
	page->operations = &anon_ops;
	
	struct anon_page *anon_page = &page->anon;
	anon_page -> swap_slot_idx = BITMAP_ERROR;
	return true;
}

/* Swap in the page by read contents from the swap disk. */
static bool
anon_swap_in (struct page *page, void *kva) {
	struct anon_page *anon_page = &page->anon;
	size_t swap_slot_idx = anon_page -> swap_slot_idx;
	printf("swap in");
	for (int i = 0; i < SECTORS_PER_PAGE; i++) {
		disk_sector_t sec_no = swap_slot_idx * SECTORS_PER_PAGE + i;
		void * buffer = kva + i * DISK_SECTOR_SIZE;
		disk_read(swap_disk, sec_no, buffer);
	}

	bitmap_flip(swap_slot, swap_slot_idx);
	anon_page -> swap_slot_idx = BITMAP_ERROR;
	return true;
}

/* Swap out the page by writing contents to the swap disk. */
static bool
anon_swap_out (struct page *page) {
	struct anon_page *anon_page = &page->anon;
	// Find free swap slot
	size_t swap_slot_idx = bitmap_scan_and_flip(swap_slot, 0, 1, false);

	if(swap_slot_idx == BITMAP_ERROR) {
		PANIC("No Free Swap Slot!");
	}

	for (int i = 0; i < SECTORS_PER_PAGE; i++) {
		disk_sector_t sec_no = swap_slot_idx * SECTORS_PER_PAGE + i;
		void * buffer = page->frame->kva + i * DISK_SECTOR_SIZE;
		disk_write(swap_disk, sec_no, buffer);
	}

	anon_page -> swap_slot_idx = swap_slot_idx;

	pml4_clear_page(thread_current() -> pml4, page->va);
	page->frame = NULL;

	return true;
}


// 3-2 start
/* Destroy the anonymous page. PAGE will be freed by the caller. */
static void
anon_destroy (struct page *page) {
	if(page != NULL){
		free(page);
	}
}
// 3-2 end