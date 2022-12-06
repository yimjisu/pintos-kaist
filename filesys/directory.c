#include "filesys/directory.h"
#include <stdio.h>
#include <string.h>
#include <list.h>
#include "filesys/filesys.h"
#include "filesys/inode.h"
#include "threads/malloc.h"

/* Creates a directory with space for ENTRY_CNT entries in the
 * given SECTOR.  Returns true if successful, false on failure. */
bool
dir_create (disk_sector_t sector, size_t entry_cnt) {
	return inode_create (sector, entry_cnt * sizeof (struct dir_entry), 1);
}

/* Opens and returns the directory for the given INODE, of which
 * it takes ownership.  Returns a null pointer on failure. */
struct dir *
dir_open (struct inode *inode) {
	struct dir *dir = calloc (1, sizeof *dir);
	if (inode != NULL && dir != NULL) {
		dir->inode = inode;
		dir->pos = 0;
		return dir;
	} else {
		inode_close (inode);
		free (dir);
		return NULL;
	}
}

/* Opens the root directory and returns a directory for it.
 * Return true if successful, false on failure. */
struct dir *
dir_open_root (void) {
	return dir_open (inode_open (ROOT_DIR_SECTOR));
}

/* Opens and returns a new directory for the same inode as DIR.
 * Returns a null pointer on failure. */
struct dir *
dir_reopen (struct dir *dir) {
	return dir_open (inode_reopen (dir->inode));
}

/* Destroys DIR and frees associated resources. */
void
dir_close (struct dir *dir) {
	if (dir != NULL) {
		inode_close (dir->inode);
		free (dir);
	}
}

/* Returns the inode encapsulated by DIR. */
struct inode *
dir_get_inode (struct dir *dir) {
	return dir->inode;
}

/* Searches DIR for a file with the given NAME.
 * If successful, returns true, sets *EP to the directory entry
 * if EP is non-null, and sets *OFSP to the byte offset of the
 * directory entry if OFSP is non-null.
 * otherwise, returns false and ignores EP and OFSP. */
static bool
lookup (const struct dir *dir, const char *name,
		struct dir_entry *ep, off_t *ofsp) {
	struct dir_entry e;
	size_t ofs;

	ASSERT (dir != NULL);
	ASSERT (name != NULL);
	
	for (ofs = 0; inode_read_at (dir->inode, &e, sizeof e, ofs) == sizeof e;
			ofs += sizeof e)
		if (e.in_use && !strcmp (name, e.name)) {
			if (ep != NULL)
				*ep = e;
			if (ofsp != NULL)
				*ofsp = ofs;
			return true;
		}
	return false;
}

/* Searches DIR for a file with the given NAME
 * and returns true if one exists, false otherwise.
 * On success, sets *INODE to an inode for the file, otherwise to
 * a null pointer.  The caller must close *INODE. */
bool
dir_lookup (const struct dir *dir, const char *name,
		struct inode **inode) {
	struct dir_entry e;

	ASSERT (dir != NULL);
	ASSERT (name != NULL);

	//P4-2 start
	if (strcmp (name, ".") == 0) {
		*inode = inode_reopen(dir->inode);
	}
	else if (strcmp (name, "..") == 0) {
		inode_read_at (dir->inode, &e, sizeof e, 0);
		*inode = inode_open(e.inode_sector);
	}
	//P4-2 end

	else if (lookup (dir, name, &e, NULL))
		*inode = inode_open (e.inode_sector);
	else
		*inode = NULL;
	return *inode != NULL;
}

/* Adds a file named NAME to DIR, which must not already contain a
 * file by that name.  The file's inode is in sector
 * INODE_SECTOR.
 * Returns true if successful, false on failure.
 * Fails if NAME is invalid (i.e. too long) or a disk or memory
 * error occurs. */
bool
dir_add (struct dir *dir, const char *name, disk_sector_t inode_sector, bool isdir) { //P4-2 : add isdir
	struct dir_entry e;
	off_t ofs;
	bool success = false;

	ASSERT (dir != NULL);
	ASSERT (name != NULL);

	/* Check NAME for validity. */
	if (*name == '\0' || strlen (name) > NAME_MAX || strlen (name) == 0) //P4-2
		return false;

	/* Check that NAME is not in use. */
	if (lookup (dir, name, NULL, NULL))
		goto done;

	//P4-2 start
	if (isdir) {
		struct dir *child_dir = dir_open( inode_open(inode_sector) );
		if(child_dir == NULL) goto done;
		e.inode_sector = inode_get_inumber( dir_get_inode(dir) );
		if (inode_write_at(child_dir->inode, &e, sizeof e, 0) != sizeof e) {
			dir_close (child_dir);
			goto done;
		}
		dir_close (child_dir);
	}
	//P4-2 end
	/* Set OFS to offset of free slot.
	 * If there are no free slots, then it will be set to the
	 * current end-of-file.

	 * inode_read_at() will only return a short read at end of file.
	 * Otherwise, we'd need to verify that we didn't get a short
	 * read due to something intermittent such as low memory. */
	for (ofs = sizeof e; inode_read_at (dir->inode, &e, sizeof e, ofs) == sizeof e;
			ofs += sizeof e)
		if (!e.in_use)
			break;

	/* Write slot. */
	e.in_use = true;
	strlcpy (e.name, name, sizeof e.name);
	e.inode_sector = inode_sector;
	success = inode_write_at (dir->inode, &e, sizeof e, ofs) == sizeof e;

done:
	return success;
}

/* Removes any entry for NAME in DIR.
 * Returns true if successful, false on failure,
 * which occurs only if there is no file with the given NAME. */
bool
dir_remove (struct dir *dir, const char *name) {
	struct dir_entry e;
	struct inode *inode = NULL;
	bool success = false;
	off_t ofs;

	ASSERT (dir != NULL);
	ASSERT (name != NULL);

	if (!(strcmp(name, ".") && strcmp(name, "..")))
		goto done;

	/* Find directory entry. */
	if (!lookup (dir, name, &e, &ofs))
		goto done;

	/* Open inode. */
	inode = inode_open (e.inode_sector);
	if (inode == NULL)
		goto done;

	//P4-2 start
	if (inode_isdir(inode)) {
		struct dir *target_dir = dir_open(inode);
		bool empty = dir_empty(target_dir);
		dir_close(target_dir);
		if (!empty) goto done;
	}
	//P4-2 end
	/* Erase directory entry. */
	e.in_use = false;
	if (inode_write_at (dir->inode, &e, sizeof e, ofs) != sizeof e)
		goto done;

	/* Remove inode. */
	inode_remove (inode);
	success = true;

done:
	inode_close (inode);
	return success;
}

/* Reads the next directory entry in DIR and stores the name in
 * NAME.  Returns true if successful, false if the directory
 * contains no more entries. */
bool
dir_readdir (struct dir *dir, char name[NAME_MAX + 1]) {
	struct dir_entry e;

	while (inode_read_at (dir->inode, &e, sizeof e, dir->pos) == sizeof e) {
		dir->pos += sizeof e;
		if (e.in_use) {
			strlcpy (name, e.name, NAME_MAX + 1);
			return true;
		}
	}
	return false;
}

//P4-2 start
void
dir_seek (struct dir *dir, off_t new_pos) {
	ASSERT (dir != NULL);
	ASSERT (new_pos >= 0);
	dir->pos = new_pos;
}

bool
dir_empty (const struct dir *dir) {
	struct dir_entry e;
	size_t ofs;

	for (ofs = 0; inode_read_at (dir->inode, &e, sizeof e, ofs) == sizeof e;
		 ofs += sizeof e) {
		if (e.in_use) return false;

	}
	return true;
}

void parse_name (const char *name, char *dir_name, char *file_name) {
	char *name_copy = (char *)malloc(strlen(name) + 1);
    strlcpy(name_copy, name, strlen(name) + 1);

	if (strlen(name_copy) == 0) {
		strlcpy(dir_name, "", 1);
        strlcpy (file_name, "", 1);
        return;
	}

	char *token, *token_next, *saveptr;
    token = strtok_r(name_copy, "/", &saveptr);
    token_next = strtok_r(NULL, "/", &saveptr);

	if (token == NULL) {
        strlcpy(dir_name, "/", 2);
        strlcpy (file_name, ".", 2);
        return;
	}

	while(true) {
		if (token_next == NULL) break;
		token = token_next;
		token_next = strtok_r(NULL, "/", &saveptr);
	}

	strlcpy (file_name, token, strlen(token) + 1);
	int dir_len = strlen(name) - strlen(token);

	if (dir_len == 0) {
		strlcpy (dir_name, "", 1);
	}
	else if (dir_len == 1) {
		strlcpy (dir_name, "/", 2);
	}
	else {
		strlcpy (dir_name, name, dir_len);
	}
    return;
}

struct dir* get_dir(char *dir_name) {
	char* temp_dir_name = (char*)malloc(sizeof(dir_name) + 1);
	strlcpy(temp_dir_name, dir_name, strlen(dir_name) + 1);

	struct dir *dir = NULL;
	struct thread *curr = thread_current();
	char *token, *saveptr;
	struct inode *inode;
	bool repeat;

	while (true) {
		repeat = false;
		if(temp_dir_name[0] == '/') { //절대경로
			dir = dir_open_root();
		}
		else { //상대경로
			if (thread_current()->working_dir == NULL) {
				dir = dir_open_root();
			}
			else dir = dir_reopen(thread_current()->working_dir);
		} 

		token = strtok_r (temp_dir_name, "/", &saveptr);

		while (token != NULL) {
			if (!dir_lookup(dir, token, &inode)) {
				dir_close(dir);
				return NULL;
			}

			if(!inode_isdir(inode) && !inode_islink(inode)) {
				dir_close(dir);
				inode_close(inode);
				return NULL;
			}

			if(inode_islink(inode)) {
				repeat = true;
				strlcpy(temp_dir_name, inode->data.link, strlen(inode->data.link) + 1);
				if (strlen(saveptr) != 0) {
					strlcat(temp_dir_name, "/", strlen(temp_dir_name) + 2);
					strlcat(temp_dir_name, saveptr, strlen(temp_dir_name) + strlen(saveptr) + 1);
				}
				dir_close(dir);
				break;
			}

			else {
				dir_close(dir);
				dir = dir_open(inode);
				token = strtok_r(NULL, "/", &saveptr);
			}
		}
		if (!repeat) break;
	}
	strlcpy(dir_name, temp_dir_name, strlen(temp_dir_name) + 1);

	if (inode_isremoved (dir_get_inode(dir))) {
		dir_close(dir);
		dir = NULL;
	}
	return dir;
}
//P4-2 end