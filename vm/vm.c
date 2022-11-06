/* vm.c: Generic interface for virtual memory objects. */

#include "threads/malloc.h"
#include "vm/vm.h"
#include "vm/inspect.h"
#include "filesys/filesys.h"

// P3-1 start
#include "hash.h"
#include "threads/mmu.h"
struct list frame_list;
// P3-1 end

/* Initializes the virtual memory subsystem by invoking each subsystem's
 * intialize codes. */
void
vm_init (void) {
	vm_anon_init ();
	vm_file_init ();
#ifdef EFILESYS  /* For project 4 */
	pagecache_init ();
#endif
	register_inspect_intr ();
	/* DO NOT MODIFY UPPER LINES. */
	/* TODO: Your code goes here. */
	// 3-1 start
	list_init (&frame_list);
	// 3-1 end
}

/* Get the type of the page. This function is useful if you want to know the
 * type of the page after it will be initialized.
 * This function is fully implemented now. */
enum vm_type
page_get_type (struct page *page) {
	int ty = VM_TYPE (page->operations->type);
	switch (ty) {
		case VM_UNINIT:
			return VM_TYPE (page->uninit.type);
		default:
			return ty;
	}
}

/* Helpers */
static struct frame *vm_get_victim (void);
static bool vm_do_claim_page (struct page *page);
static struct frame *vm_evict_frame (void);

// P3-2 start
/* Create the pending page object with initializer. If you want to create a
 * page, do not create it directly and make it through this function or
 * `vm_alloc_page`. */
bool
vm_alloc_page_with_initializer (enum vm_type type, void *upage, bool writable,
		vm_initializer *init, void *aux) {

	ASSERT (VM_TYPE(type) != VM_UNINIT)

	struct supplemental_page_table *spt = &thread_current ()->spt;

	/* Check wheter the upage is already occupied or not. */
	if (spt_find_page (spt, upage) == NULL) {
		/* TODO: Create the page, fetch the initialier according to the VM type,
		 * TODO: and then create "uninit" page struct by calling uninit_new. You
		 * TODO: should modify the field after calling the uninit_new. */
		struct page *p = malloc(sizeof(struct page));
		switch (VM_TYPE(type)){
			case VM_ANON:
				/* code */
				uninit_new(p, upage, init, type, aux, anon_initializer);
				break;
			case VM_FILE:
				uninit_new(p, upage, init, type, aux, file_backed_initializer);
				break;
		}

		p -> writable = writable;

		/* TODO: Insert the page into the spt. */
		return spt_insert_page(spt, p);
	}
err:
	return false;
}
// P3-2 end

// P3-1 start
/* Find VA from spt and return page. On error, return NULL. */
struct page *
spt_find_page (struct supplemental_page_table *spt UNUSED, void *va UNUSED) {
	struct page *page;
	/* TODO: Fill this function. */
	page = malloc(sizeof(struct page));
	page->va = pg_round_down(va);
	struct hash_elem *e = hash_find(&spt->spt_hash, &page->hash_elem);

	if(e == NULL) {
		return NULL;
	}
	return hash_entry(e, struct page, hash_elem);
	
}

/* Insert PAGE into spt with validation. */
bool
spt_insert_page (struct supplemental_page_table *spt UNUSED,
		struct page *page UNUSED) {
	int succ = false;
	/* TODO: Fill this function. */
	if(hash_insert(&spt->spt_hash, &page->hash_elem) == NULL){
		succ = true;
	}
	return succ;
}
// P3-1 end

// P3-2 start
/* Get the struct frame, that will be evicted. */
static struct frame *
vm_get_victim (void) {
	struct frame *victim = NULL;
	 /* TODO: The policy for eviction is up to you. */
	
	struct list_elem *e;
	struct thread *curr = thread_current();

	for(e = list_begin(&frame_list); e < list_end(&frame_list); e = list_next(e)) {
		victim = list_entry(e, struct frame, frame_elem);
		if (!pml4_is_accessed(curr->pml4, victim->page->va)) {
			break;
		}
		pml4_set_accessed(curr->pml4, victim->page->va, false);
	}
	
	return victim;
}

/* Evict one page and return the corresponding frame.
 * Return NULL on error.*/
static struct frame *
vm_evict_frame (void) {
	struct frame *victim UNUSED = vm_get_victim ();
	/* TODO: swap out the victim and return the evicted frame. */
	if(victim == NULL) {
		return NULL;
	}
	swap_out(victim -> page);
	return victim;
}

/* palloc() and get frame. If there is no available page, evict the page
 * and return it. This always return valid address. That is, if the user pool
 * memory is full, this function evicts the frame to get the available memory
 * space.*/
static struct frame *
vm_get_frame (void) {
	struct frame *frame = NULL;
	/* TODO: Fill this function. */
	frame = malloc(sizeof(struct frame));	
	frame->page = NULL;

	ASSERT (frame != NULL);
	ASSERT (frame->page == NULL);

	struct page *p = palloc_get_page(PAL_USER);
	if (p == NULL) {
		free(frame);
		frame = vm_evict_frame();
	}else {
		frame -> kva = p;
	}
	
	return frame;
}

/* Growing the stack. */
static void
vm_stack_growth (void *addr UNUSED) {
}

/* Handle the fault on write_protected page */
static bool
vm_handle_wp (struct page *page UNUSED) {
	return false;
}

/* Return true on success */
bool
vm_try_handle_fault (struct intr_frame *f UNUSED, void *addr UNUSED,
		bool user UNUSED, bool write UNUSED, bool not_present UNUSED) {
	struct supplemental_page_table *spt UNUSED = &thread_current()->spt;
	struct page *page = NULL;
	/* TODO: Validate the fault */
	if (user && is_kernel_vaddr(addr))  {
		return false;
	}
	// Start P3-4
	// Check Whether the page fault is valid case for stack growth or not.
	void *rsp = thread_current() -> rsp;
	if (rsp - PGSIZE <= addr )
	// End P3-4

	/* TODO: Your code goes here */
	page = spt_find_page(spt, addr);
	if(page == NULL) {
		return false;
	}
	
	if(write && !not_present) {
		return vm_handle_wp(page);
	}

	return vm_do_claim_page (page);
}

// P3-2 end

/* Free the page.
 * DO NOT MODIFY THIS FUNCTION. */
void
vm_dealloc_page (struct page *page) {
	destroy (page);
	free (page);
}

// P3-1 start
/* Claim the page that allocate on VA. */
// Allocate a physical frame for VA page.
bool
vm_claim_page (void *va UNUSED) {
	struct page *page = NULL;
	/* TODO: Fill this function */
	struct thread *curr = thread_current();

	page = spt_find_page(&curr->spt, va);
	if(page == NULL) {
		return false;
	}
	return vm_do_claim_page (page);
}

/* Claim (allocate physical frame) the PAGE and set up the mmu. */
static bool
vm_do_claim_page (struct page *page) {
	struct frame *frame = vm_get_frame ();

	/* Set links */
	frame->page = page;
	page->frame = frame;

	/* TODO: Insert page table entry to map page's VA to frame's PA. */
	struct thread *t = thread_current ();
	list_push_back(&frame_list, &frame->frame_elem);
	/* Verify that there's not already a page at that virtual
	 * address, then map our page there. */

	if (pml4_get_page (t->pml4, page->va) == NULL
			&& pml4_set_page (t->pml4, page->va, frame->kva, page->writable))
		return swap_in (page, frame->kva); // WHY??
	return false;
}

/* Initialize new supplemental page table */

/* Computes and returns the hash value for hash element E, given
 * auxiliary data AUX. */
void hash_func(const struct hash_elem *e, void *aux) {
	struct page *p = hash_entry(e, struct page, hash_elem);
	return hash_bytes(&p->va, sizeof p->va);
}

/* Compares the value of two hash elements A and B, given
 * auxiliary data AUX.  Returns true if A is less than B, or
 * false if A is greater than or equal to B. */
bool less_func(const struct hash_elem *a, const struct hash_elem *b, void *aux) {
	struct page *page_a = hash_entry(a, struct page, hash_elem);
	struct page *page_b = hash_entry(b, struct page, hash_elem);
	return page_a->va < page_b->va;
}

void
supplemental_page_table_init (struct supplemental_page_table *spt UNUSED) {
	hash_init(&spt->spt_hash, hash_func, less_func, NULL);
}

// P3-1 end

// P3-2 start
/* Copy supplemental page table from src to dst */
bool
supplemental_page_table_copy (struct supplemental_page_table *dst UNUSED,
		struct supplemental_page_table *src UNUSED) {

	struct hash *h = &src->spt_hash;
	struct hash_iterator i;

	hash_first (&i, h);
	while (hash_next (&i))
	{
		struct page *page = hash_entry (hash_cur (&i), struct page, hash_elem);

		enum vm_type type = page_get_type(page);
		void *upage = page->va;
		bool writable = page->writable;

		if(page->operations->type == VM_UNINIT) {
			vm_initializer *init = page->uninit.init;
			void *aux = page->uninit.aux;
			if(page->uninit.type & VM_ANON) {
				if(!vm_alloc_page_with_initializer(type, upage, writable, init, aux)){
					return false;
				};
			}
		}else {
			if(!vm_alloc_page(type, upage, writable)) {
				return false;
			}
			if(!vm_claim_page(upage)){
				return false;
			}
			struct page *new_page = spt_find_page(dst, upage);
			memcpy(new_page->frame->kva, page->frame->kva, PGSIZE);
		}
		
	}
	return true;
}

/* Free the resource hold by the supplemental page table */
void 
spt_destroy (struct hash_elem *e, void *aux) {
	struct page *page = hash_entry (e, struct page, hash_elem);
	if(page != NULL){
		destroy(page);
	}
}

void
supplemental_page_table_kill (struct supplemental_page_table *spt UNUSED) {
	/* TODO: Destroy all the supplemental_page_table hold by thread and
	 * TODO: writeback all the modified contents to the storage. */
	if(hash_empty(&spt->spt_hash)) {
		return;
	}
	hash_destroy(&spt->spt_hash, spt_destroy);
}
// P3-2 end