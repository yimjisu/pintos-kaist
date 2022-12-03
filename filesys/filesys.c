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
    //P4-2 start
    char *file_name = (char *)malloc(strlen(name) + 1);
    struct dir *dir = parse_path(name, file_name);
    if (dir == NULL) return false;
    //P4-2 end
	
	// P4-1 start
    cluster_t inode_cluster = 0;
    inode_cluster = fat_create_chain(inode_cluster);
    if (inode_cluster == 0) return false;
  
    bool success = (inode_create(inode_cluster, initial_size, 0) && dir_add(dir, file_name, cluster_to_sector(inode_cluster), 0));
    
    if (!success) fat_remove_chain(inode_cluster, 0);
	// P4-1 end
	dir_close (dir);
	return success;
}

//P4-2 start
bool
filesys_create_dir(const char* name) {
    //P4-2 start
    char *file_name = (char *)malloc(strlen(name) + 1);
    struct dir *dir = parse_path(name, file_name);
    if (dir == NULL) return false;
    //P4-2 end
	
	// P4-1 start
    cluster_t inode_cluster = 0;
    inode_cluster = fat_create_chain(inode_cluster);
    if (inode_cluster == 0) return false;
    
    bool success = (
        dir_create(inode_cluster, 0)
        && dir_add(dir, file_name, cluster_to_sector(inode_cluster), 1));
    
    if (!success) fat_remove_chain(inode_cluster, 0);
	// P4-1 end
	dir_close (dir);
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
    struct inode *inode = NULL;
    char* name_cpy = (char *)malloc(strlen(name) + 1);
    strlcpy(name_cpy, name, strlen(name) + 1);

    char* file_name = (char *)malloc(strlen(name) + 1);

    while (true) {
        struct dir *dir = parse_path(name_cpy, file_name);
        if (dir == NULL) break;
        if (!dir_lookup(dir, file_name, &inode)) {
            dir_close(dir);
            break;
        }
        dir_close(dir);
        if(!(inode && inode->data.islink)) break;
        name_cpy = inode->data.link;
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
    char* file_name = (char *)malloc(strlen(name) + 1);
    struct dir* dir = parse_path(name, file_name);

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
    // if (!dir_create(ROOT_DIR_SECTOR, 16)) {
    //     PANIC("root directory creation failed");
    // }
    if (!inode_create_root(ROOT_DIR_SECTOR, 16)) {
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

struct dir*
get_dir(char *name) {
	if(name == NULL) return NULL;

	struct dir *dir = NULL;
	struct thread *curr = thread_current();
    if(name[0] == '/') { //절대경로
        dir = dir_open_root();
    }
    else { //상대경로
        if (thread_current()->working_dir == NULL) {
			dir = dir_open_root();
		}
        else dir = dir_reopen(thread_current()->working_dir);
	}
    
    return dir;
};

bool
filesys_chdir(const char *name) {
    struct dir *dir = get_dir(name);
	struct inode *inode = NULL;
    
	char *name_cpy = (char *) malloc(strlen(name) + 1);
	strlcpy(name_cpy, name, strlen(name) + 1);
	char *token, *saveptr;
	for(token = strtok_r(name_cpy, "/", &saveptr); token != NULL; 
	token = strtok_r(NULL, "/", &saveptr)) {
		if (!dir_lookup(dir, token, &inode) || !(inode_isdir(inode))) {
			dir_close(dir);
			return NULL;
		}
		dir_close(dir);
		dir = dir_open(inode);
	}

    if (dir == NULL) return false;
	struct thread *curr = thread_current();
    dir_close(curr->working_dir);
	curr->working_dir = dir;
    return true;
}


//P4-2 end

struct dir*
parse_path(char *name, char *file_name) {
    if(name == NULL) return NULL;
    
    if(strlen(name) == 0) {
        strlcpy(file_name, "", 1);
        return  dir_open_root();
    }

    struct dir *dir = get_dir(name);

    char *name_cpy = (char *) malloc(strlen(name) + 1);
	strlcpy(name_cpy, name, strlen(name) + 1);
    struct inode *inode = NULL;

	char *token, *saveptr, *token_next;
    token = strtok_r(name_cpy, "/", &saveptr);
    token_next = strtok_r(NULL, "/", &saveptr);

    
    while(token != NULL && token_next != NULL){
		if (!dir_lookup(dir, token, &inode)) {
			dir_close(dir);
			return NULL;
		}

        if(inode -> data.islink) {
            char *link_cpy = (char *) malloc(strlen(inode->data.link) + 1);
	        strlcpy(link_cpy, inode->data.link, strlen(inode->data.link) + 1);
            dir_close(dir);
            dir = get_dir(link_cpy);
        
            strlcat(link_cpy, "/", strlen(link_cpy) + 2);
            strlcat(link_cpy, token_next, strlen(link_cpy) + strlen(token_next) + 1);
            strlcat(link_cpy, saveptr, strlen(link_cpy) + strlen(saveptr) + 1);
            
            token = strtok_r(link_cpy, "/", &saveptr);
            token_next = strtok_r(NULL, "/", &saveptr);
            continue;
        }

		dir_close(dir);
		dir = dir_open(inode);
        token = token_next;
        token_next = strtok_r(NULL, "/", &saveptr);
        
        if(!inode_isdir(inode)) {
            dir_close(dir);
            inode_close(inode);
            return NULL;
        }
	}
    
    if (inode_isremoved (dir_get_inode(dir))) {
        dir_close(dir);
        dir = NULL;
    }
    
    if(token == NULL) {
        strlcpy(file_name, ".", 2);
    }else {
        strlcpy (file_name, token, strlen(token) + 1);
    }
    return dir;
}