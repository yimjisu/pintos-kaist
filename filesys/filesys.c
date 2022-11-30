#include "filesys/filesys.h"
#include <debug.h>
#include <stdio.h>
#include <string.h>
#include "filesys/file.h"
#include "filesys/free-map.h"
#include "filesys/inode.h"
#include "filesys/directory.h"
#include "devices/disk.h"
#include "filesys/fat.h"
#include "threads/thread.h"

/* The disk that contains the file system. */
struct disk *filesys_disk;

static void do_format (void);

/* Initializes the file system module.
 * If FORMAT is true, reformats the file system. */
void
filesys_init (bool format) {
	filesys_disk = disk_get (0, 1);
	if (filesys_disk == NULL)
		PANIC ("hd0:1 (hdb) not present, file system initialization failed");

	inode_init ();

#ifdef EFILESYS
	fat_init ();

	if (format)
		do_format ();

	fat_open ();
	thread_current()->working_dir = dir_open_root();//P4-2
#else
	/* Original FS */
	free_map_init ();

	if (format)
		do_format ();

	free_map_open ();
#endif
    struct dir* root_dir = dir_open_root();
    dir_add(root_dir, ".", ROOT_DIR_SECTOR);
    dir_add(root_dir, "..", ROOT_DIR_SECTOR);
    dir_close(root_dir);
}

/* Shuts down the file system module, writing any unwritten data
 * to disk. */
void
filesys_done (void) {
	/* Original FS */
#ifdef EFILESYS
	fat_close ();
#else
	free_map_close ();
#endif
}

/* Creates a file named NAME with the given INITIAL_SIZE.
 * Returns true if successful, false otherwise.
 * Fails if a file named NAME already exists,
 * or if internal memory allocation fails. */
bool
filesys_create (const char *name, off_t initial_size) {
	cluster_t inode_cluster = 0;
    //P4-2 start
    char *path_name = (char *)malloc(strlen(name) + 1);
    strlcpy(path_name, name, strlen(name) + 1);

    char *file_name = (char *)malloc(strlen(name) + 1);
    struct dir *dir = parse_path(path_name, file_name);
    //P4-2 end
	
	// P4-1 start
    // struct dir *dir = dir_open_root ();
	bool success = false;
    inode_cluster = fat_create_chain(inode_cluster);

    success = (dir != NULL
               && inode_create(inode_cluster, initial_size, 0)
               && dir_add(dir, file_name, cluster_to_sector(inode_cluster)));
    
    if (!success && inode_cluster != 0) {
        fat_remove_chain(inode_cluster, 0);
    }
	// P4-1 end
	dir_close (dir);
	return success;
}

//P4-2 start
bool
filesys_create_dir(const char* name) {
	disk_sector_t inode_sector = 0;
    bool success = false;
    char* path_name = (char *)malloc(strlen(name) + 1);
    strlcpy(path_name, name, strlen(name) + 1);

    char* file_name = (char *)malloc(strlen(name) + 1);
    struct dir* dir = parse_path(path_name, file_name);

    inode_sector = fat_create_chain(inode_sector);
    struct inode *sub_dir_inode;
    struct dir *sub_dir = NULL;

    success = (
                dir != NULL
            	&& dir_create(inode_sector, 16)
            	&& dir_add(dir, file_name, cluster_to_sector(inode_sector))
            	&& dir_lookup(dir, file_name, &sub_dir_inode)
            	&& dir_add(sub_dir = dir_open(sub_dir_inode), ".", cluster_to_sector(inode_sector))
            	&& dir_add(sub_dir, "..", cluster_to_sector(inode_get_inumber(dir_get_inode(dir))))
            );

    if (!success && inode_sector != 0) {
        fat_remove_chain(inode_sector, 0);
	}
    dir_close(sub_dir);
    dir_close(dir);
    return success;
}
//P4-2 end

/* Opens the file with the given NAME.
 * Returns the new file if successful or a null pointer
 * otherwise.
 * Fails if no file named NAME exists,
 * or if an internal memory allocation fails. */
struct file *
filesys_open (const char *name) {
    //P4-2 start
    char* path_name = (char *)malloc(strlen(name) + 1);
    strlcpy(path_name, name, strlen(name) + 1);
    char* file_name = (char *)malloc(strlen(name) + 1);

    struct dir* dir = NULL;
    struct inode *inode = NULL;

    while (true) {
        dir = parse_path(path_name, file_name);
        if (dir != NULL) {
            dir_lookup(dir, file_name, &inode);
            dir_close(dir);
            if(!(inode && inode->data.islink)) break;
            path_name = inode->data.link;
        }
    }
    //P4-2 end
	return file_open (inode);
}

/* Deletes the file named NAME.
 * Returns true if successful, false on failure.
 * Fails if no file named NAME exists,
 * or if an internal memory allocation fails. */
bool
filesys_remove (const char *name) {
    //P4-2 start
    char* path_name = (char *)malloc(strlen(name) + 1);
    strlcpy(path_name, name, strlen(name) + 1);

    char* file_name = (char *)malloc(strlen(name) + 1);
    struct dir* dir = parse_path(path_name, file_name);

    bool success = false;

    if (dir != NULL) {
        success = dir_remove(dir, file_name);
    }
    //P4-2 end
	dir_close (dir);
	return success;
}

/* Formats the file system. */
static void
do_format (void) {
	printf ("Formatting file system...");

#ifdef EFILESYS
	/* Create FAT and save it to the disk. */
	fat_create ();
    //P4-2 start
    if (!dir_create(ROOT_DIR_SECTOR, 16)) {
        PANIC("root directory creation failed");
    }
    //P4-2 end
	fat_close ();
#else
	free_map_create ();
	if (!dir_create (ROOT_DIR_SECTOR, 16))
		PANIC ("root directory creation failed");
	free_map_close ();
#endif

	printf ("done.\n");
}

//P4-2 start
struct dir *parse_path(char *path_name, char *file_name) {
    struct dir *dir = NULL;
	struct thread *curr = thread_current();

    if (path_name == NULL || file_name == NULL) return NULL;

    if (strlen(path_name) == 0) {
        strlcpy(file_name, path_name, strlen(path_name) + 1);
        dir = dir_open_root();
        return dir;
    }

    if(path_name[0] == '/') { //절대경로
        dir = dir_open_root();
    }
    else { //상대경로
        if (curr->working_dir == NULL) dir = dir_open_root();
        else dir = dir_reopen(curr->working_dir);
	}

    char *token, *token_next, *saveptr;
    token = strtok_r(path_name, "/", &saveptr);
    token_next = strtok_r(NULL, "/", &saveptr);

    // "/"인 경우
    if(token == NULL) {
        token = (char*)malloc(2);
        strlcpy(token, ".", 2);
    }

    struct inode *inode;
    while (token != NULL && token_next != NULL) {
        if (!dir_lookup(dir, token, &inode)) {
            dir_close(dir);
            return NULL;
        }

        if(inode->data.islink) {
            char* new_path = (char*)malloc(sizeof(strlen(inode->data.link)) + 1);
            strlcpy(new_path, inode->data.link, strlen(inode->data.link) + 1);

            strlcpy(path_name, new_path, strlen(new_path) + 1);
            free(new_path);
 
            strlcat(path_name, "/", strlen(path_name) + 2);
            strlcat(path_name, token_next, strlen(path_name) + strlen(token_next) + 1);
            strlcat(path_name, saveptr, strlen(path_name) + strlen(saveptr) + 1);

            dir_close(dir);

            if(path_name[0] == '/') {
                dir = dir_open_root();
            }
            else {
                dir = dir_reopen(curr->working_dir);
            }

            token = strtok_r(path_name, "/", &saveptr);
            token_next = strtok_r(NULL, "/", &saveptr);
            continue;
        }
        
        dir_close(dir);
        dir = dir_open(inode);

        token = token_next;
        token_next = strtok_r(NULL, "/", &saveptr);
        
        if (token_next != NULL) {
            if(!inode_isdir(inode)) {
                dir_close(dir);
                inode_close(inode);
                return NULL;
            }
        }
        else break;
    }

    strlcpy (file_name, token, strlen(token) + 1);

    return dir;
}

bool
filesys_chdir(const char *name) {
    struct thread *curr = thread_current();
    if (name == NULL) return false;

    char *name_copy = (char *)malloc(strlen(name) + 1);
    strlcpy(name_copy, name, strlen(name) + 1);

    struct dir *chdir = NULL;

    if (name_copy[0] == '/') { //절대경로
        chdir = dir_open_root();
    }
    else { //상대경로
        chdir = dir_reopen(curr->working_dir);
	}

    char *token, *saveptr;
    token = strtok_r(name_copy, "/", &saveptr);

    struct inode *inode = NULL;
    while (token != NULL) {
        if (!dir_lookup(chdir, token, &inode)) {
            dir_close(chdir);
            return false;
        }
        if (!inode_isdir(inode)) { //file인 경우
            dir_close(chdir);
            return false;
        }
        dir_close(chdir);
        chdir = dir_open(inode);
        token = strtok_r(NULL, "/", &saveptr);
    }

    dir_close(curr->working_dir);
	curr->working_dir = chdir;
    return true;
}
//P4-2 end