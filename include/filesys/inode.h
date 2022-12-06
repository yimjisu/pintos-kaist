#ifndef FILESYS_INODE_H
#define FILESYS_INODE_H

#include <stdbool.h>
#include "filesys/off_t.h"
#include "devices/disk.h"
#include <list.h>

struct bitmap;

/* On-disk inode.
 * Must be exactly DISK_SECTOR_SIZE bytes long. */
struct inode_disk {
	disk_sector_t start;                /* First data sector. */
	off_t length;                       /* File size in bytes. */
	unsigned magic;                     /* Magic number. */
	uint32_t isdir; //P4-2
    uint32_t islink; //P4-2
	// uint32_t unused[123]; //P4-2        /* Not used. */
    char link[492];
};

/* In-memory inode. */
struct inode {
	struct list_elem elem;              /* Element in inode list. */
	disk_sector_t sector;               /* Sector number of disk location. */
	int open_cnt;                       /* Number of openers. */
	bool removed;                       /* True if deleted, false otherwise. */
	int deny_write_cnt;                 /* 0: writes ok, >0: deny writes. */
	struct inode_disk data;             /* Inode content. */
};



void inode_init (void);
bool inode_create (disk_sector_t, off_t, uint32_t);
bool inode_create_root (disk_sector_t, off_t);//P4-2
bool inode_create_link (disk_sector_t, char* path_name);//P4-2
struct inode *inode_open (disk_sector_t);
struct inode *inode_reopen (struct inode *);
disk_sector_t inode_get_inumber (const struct inode *);
void inode_close (struct inode *);
void inode_remove (struct inode *);
off_t inode_read_at (struct inode *, void *, off_t size, off_t offset);
off_t inode_write_at (struct inode *, const void *, off_t size, off_t offset);
void inode_deny_write (struct inode *);
void inode_allow_write (struct inode *);
off_t inode_length (const struct inode *);
bool inode_isdir(const struct inode *);//P4-2
bool inode_isremoved (const struct inode *);//P4-2
bool inode_islink (const struct inode *);//P4-2
#endif /* filesys/inode.h */
