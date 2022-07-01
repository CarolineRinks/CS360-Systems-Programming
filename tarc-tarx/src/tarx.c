/* Caroline Rinks
 * Lab 4 Part 2 (tarx)
 * CS360 Fall 2021
 *
 * Tarx recreates a directory from a tarfile, including all 
 * file and directory protections and modification times. 
 * The syntax of tarx:
 *    ./tarx < file.tarc
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include "dllist.h"
#include "jrb.h"
#include "jval.h"

typedef struct File_Info {
	int * name_sz;
    char * filename;
    ino_t * inode;
    mode_t * mode;
    time_t * mod_time;
    off_t * file_sz;
	char * file_contents;

} File_Info;


/* @brief: Creates a new instance of a File_Info struct.
 * @return: A new File_Info instance.
*/
File_Info *new_file_info() {
	
	File_Info * file_info;
	file_info = (File_Info *) malloc(sizeof(struct File_Info));

    file_info->name_sz = (int *) malloc (sizeof(int *));
	file_info->inode = (ino_t *) malloc (sizeof(ino_t *));
    file_info->mode = (mode_t *) malloc (sizeof(mode_t *));
    file_info->mod_time = (time_t *) malloc (sizeof(time_t *));
    file_info->file_sz = (off_t *) malloc (sizeof(off_t *));

	return file_info;
}

/* @brief: Frees memory allocated for all File_Info instances.
 * @param[in] free_list: A list of File_Info instances to free.
 * @return: none
*/
void free_file_info(Dllist free_list) {
	File_Info * file_info;
	Dllist tmp;

	dll_traverse(tmp, free_list) {
		file_info = (File_Info *) tmp->val.v;
		free(file_info->name_sz);
		free(file_info->filename);
		free(file_info->inode);
		free(file_info->mode);
		free(file_info->mod_time);
		
		if (file_info->file_sz != NULL) {
			free(file_info->file_sz);
		}
		if (file_info->file_contents != NULL) {
			free(file_info->file_contents);
		}

		free(file_info);
	}
	free_dllist(free_list);
}

/* @brief: Modifies the mode and modification time of directories in the given list.
 * @param[in] files: A double-linked list that holds the directories to modify.
 * @return: none.
*/
void set_dir_modes_times(Dllist files) {
	Dllist tmp;
	File_Info * file_info;
	struct timeval times[2];

	times[0].tv_sec = (long) NULL;
	times[0].tv_usec = (long) NULL;

	dll_traverse(tmp, files) {
		file_info = (File_Info *) tmp->val.v;
		if (chmod(file_info->filename, *file_info->mode) < 0) {
			perror("chmod");
			exit(EXIT_FAILURE);
		}

		times[1].tv_sec = *file_info->mod_time;
        times[1].tv_usec = 0;
		if (utimes(file_info->filename, times) < 0) {
            perror("utimes");
            exit(EXIT_FAILURE);
        }
	}
}

/* @brief: Modifies the modification time of non-directory files.
 * @param[in] inodes: A red-black tree holding the inodes of all files that were read.
 * @return: none
*/
void set_file_times(JRB inodes) {
	JRB tmp;
	File_Info * file_info;
    struct timeval times[2];

	times[0].tv_sec = (long) NULL;
    times[0].tv_usec = (long) NULL;

	jrb_rtraverse(tmp, inodes) {
		file_info = (File_Info *) tmp->val.v;
		if (S_ISDIR(*file_info->mode)) {
			continue;
		}
		
		times[1].tv_sec = *file_info->mod_time;
		times[1].tv_usec = 0;

		if (utimes(file_info->filename, times) < 0) {
			perror("utimes");
			exit(EXIT_FAILURE);
		}
	}
}

/* @brief: Extracts info about files specified in .tarc file and uses the info to recreate the files.
 * @return: none
*/
void extract_file_info(JRB inodes, Dllist files, Dllist free_list) {

	File_Info * file_info, * tmp;
	JRB inode;
	FILE * fd;
	int num;

	while (1) {
		file_info = new_file_info();

		/* Read the file's name size, name, and inode into file_info struct */
		num = fread(file_info->name_sz, 1, sizeof(int), stdin);
		if (num == 0) {  break; }
		else if (num != sizeof(int)) { perror("fread(file_info->name_sz)"); exit(1); }

		file_info->filename = (char *) malloc (*file_info->name_sz + sizeof(char));
		num = fread(file_info->filename, sizeof(char), *file_info->name_sz, stdin);
		if (num != *file_info->name_sz) { perror("fread(file_info->filename)"); exit(1); }
		file_info->filename[*file_info->name_sz] = '\0';

		num = fread(file_info->inode, sizeof(ino_t), 1, stdin);
		if (num != 1) {	perror("fread(file_info->inode)"); exit(1); }

		/* If seeing this inode for 1st time, read its mode and mod_time
		 * from .tarc file, otherwise read it from a previous instance of the inode */
		inode = jrb_find_int(inodes, *file_info->inode);
		if (inode == NULL) {
			num = fread(file_info->mode, sizeof(mode_t), 1, stdin);
			if (num != 1) {	perror("fread(file_info->mode)"); exit(1); }
			
			num = fread(file_info->mod_time, sizeof(time_t), 1, stdin);
			if (num != 1) {	perror("fread(file_info->mod_time)"); exit(1); }

			if (S_ISDIR(*file_info->mode)) {
				dll_prepend(files, new_jval_v((void *)file_info));
				file_info->file_sz = NULL;
				file_info->file_contents = NULL;

				/* Create the directory */
				if (mkdir(file_info->filename, *file_info->mode) < 0) {
					perror(file_info->filename);
					exit(EXIT_FAILURE);
				}
				/* Temporarily change directory's mode to give all permissions */
				if (chmod(file_info->filename, 0755) < 0) {
					perror("chmod");
					exit(EXIT_FAILURE);
				}
			}
			else {
				/* Read file's size and contents into file_info struct */
				num = fread(file_info->file_sz, sizeof(off_t), 1, stdin);
				if (num != 1) {	perror("fread(file_info->file_sz)"); exit(1); }
				
				file_info->file_contents = (char *) malloc(*file_info->file_sz);
				num = fread(file_info->file_contents, sizeof(char), *file_info->file_sz, stdin);
				if (num != *file_info->file_sz) { perror("fread(file_info->file_contents)"); exit(1); }
			
				/* Create File and change its mode*/
				fd = fopen(file_info->filename, "w");
				fwrite(file_info->file_contents, sizeof(char), *file_info->file_sz, fd);
				fclose(fd);
				
				if (chmod(file_info->filename, *file_info->mode) < 0) {
					perror("chmod");
					exit(EXIT_FAILURE);
				}
			}
			
			jrb_insert_int(inodes, *file_info->inode, new_jval_v((void*)file_info));
        }
		else {
			tmp = (File_Info *) inode->val.v;
			*file_info->mode = *tmp->mode;
			*file_info->mod_time = *tmp->mod_time;
			
			if (link(tmp->filename, file_info->filename)) {
				perror("link");
				exit(EXIT_FAILURE);
			}
			
			if (S_ISDIR(*tmp->mode)) {
				dll_prepend(files, new_jval_v((void *)file_info));
				file_info->file_sz = NULL;
				file_info->file_contents = NULL;
			}
			else{
				*file_info->file_sz = *tmp->file_sz;
				file_info->file_contents = (char *) malloc(*file_info->file_sz);
				*file_info->file_contents = *tmp->file_contents;
			}
		}
		dll_append(free_list, new_jval_v((void *)file_info));
	}
}

int main(int argc, char* argv[]) {
    
	JRB inodes;
	Dllist files, free_list;
	
    inodes = make_jrb();
	files = new_dllist();
    free_list = new_dllist();

	extract_file_info(inodes, files, free_list);
	
	set_dir_modes_times(files);
    set_file_times(inodes);

    free_file_info(free_list);
    free_dllist(files);
    jrb_free_tree(inodes);

	return 0;
}
