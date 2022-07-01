/* Caroline Rinks
 * Lab 4 Part 1 (tarc)
 * CS360 Fall 2021
 *
 * The program tarc stands for "tar create" and works similarly to "tar cf".
 * Syntax of tarc:
 *     ./tarc directory > directory.tarc
*/

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include "fields.h"
#include "jrb.h"
#include "dllist.h"

/* @brief: Removes prefix from the path of the originally specified directory.
 * @param[in] full_path: The path from which to remove the prefix.
 * @param[in] dir: The directory included in the path.
 * @param[out] first_dir: The index of the directory.
 * @return: A c-string of the path with its prefix removed.
*/
char *remove_prefix(char * full_path, char * dir, int * first_dir) {	
	
	char * suffix;

	if (*first_dir == -1) {
		suffix = strrchr(full_path, '/') + sizeof(char);
		if (suffix == NULL) {
			*first_dir = 0;
			return dir;
		}

		*first_dir = suffix - full_path;	// Save index of initial directory
		return suffix;
	}

	suffix = strrchr(full_path, '/') + sizeof(char);
    if (suffix == NULL) {
		perror("something went wrong");
		exit(EXIT_FAILURE);
    }

	return suffix;
}

/* @brief: Reads files from a specified directory and writes information about each file to a .tarc file.
 * @param[in] dir: The directory being traversed.
 * @param[in] inodes: A red-black tree that holds the inodes of files that have been read.
 * @param[in] start_dir: An integer that indicates if the directory being traversed is the 1st one.
 * @param[out] first_dir: The starting position of a directory.
 * @return: none.
*/
void traverse_dir(char * dir, JRB inodes, int start_dir, int * first_dir) {

	DIR * d;
	FILE * fd;
	struct dirent * de;
	struct stat stat_buf;
	Dllist directories, tmp;
	ino_t * inode;
	mode_t * mode;
	time_t * mod_time;
	off_t * file_sz;
	int exists, num_read;
	int * name_sz;
	char * s, * file_buf, * suffix, * name;
	
	d = opendir(dir);
	if (d == NULL) {
		perror(dir);
		exit(EXIT_FAILURE);
	}

	/* Allocate Memory */
	inode = (ino_t *) malloc (sizeof(ino_t*));
	mode = (mode_t *) malloc (sizeof(mode_t*));
	mod_time = (time_t *) malloc (sizeof(time_t*));
	file_sz = (off_t *) malloc (sizeof(off_t*));
	name_sz = (int *) malloc (sizeof(int*));

	/* Set up directory list */
	directories = new_dllist();

	/* Read initial directory */
	if (start_dir == 1) {
		de = readdir(d);
		
		/* Store path of directory in s */
		s = (char *) malloc (strlen(dir) + sizeof(char));
		sprintf(s, "%s", dir);
		
		exists = lstat(s, &stat_buf);
		if (exists < 0) {
			perror(s);
			exit(EXIT_FAILURE);
		}
			
		/* Get rid of prefix in directory path b4 writing it */
		suffix = remove_prefix(s, dir, first_dir);

		/* Write file info to .tarc file */
		*name_sz = (int) strlen(suffix);
		fwrite((int *)name_sz, sizeof(int), 1, stdout);
		fwrite(suffix, strlen(suffix), 1, stdout);
		
		*inode = stat_buf.st_ino;
		fwrite(inode, sizeof(ino_t), 1, stdout);

		*mode = stat_buf.st_mode;
		fwrite(mode, sizeof(mode_t), 1, stdout);
		
		*mod_time = stat_buf.st_mtime;
		fwrite(mod_time, sizeof(time_t), 1, stdout);

		jrb_insert_int(inodes, stat_buf.st_ino, JNULL);
		free(s);
	}

	/* Read each file under current directory via recursive directory traversal */
	for (de = readdir(d); de != NULL; de = readdir(d)) {   
		if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0) {
			continue;
		}
		
		/* Store file path in s */
		s = (char *) malloc (strlen(dir) + strlen(de->d_name) + sizeof(char)*2);
		sprintf(s, "%s/%s", dir, de->d_name);

		/* Get file info with lstat() system call */
		exists = lstat(s, &stat_buf);
		if (exists < 0) {
			perror(s);
			exit(EXIT_FAILURE);
		}
		else {
			/* Remove prefix from file's name b4 writing */
			suffix = remove_prefix(s, dir, first_dir);

			name = (char *) malloc(strlen(suffix) + strlen(dir+*first_dir) + sizeof(char)*2);
			sprintf(name, "%s/%s", dir+*first_dir, suffix);

			/* Write file info to .tarc file */
			*name_sz = (int) strlen(name);
			fwrite((int *)name_sz, sizeof(int), 1, stdout);
			fwrite(name, strlen(name), 1, stdout);
			free(name);

			*inode = stat_buf.st_ino;
			fwrite(inode, sizeof(ino_t), 1, stdout);

			/* Write mode and modification time if this is the first time seeing this inode */
			if (jrb_find_int(inodes, stat_buf.st_ino) == NULL) {
				*mode = stat_buf.st_mode;
				fwrite(mode, sizeof(mode_t), 1, stdout);

				*mod_time = stat_buf.st_mtime;
				fwrite(mod_time, sizeof(time_t), 1, stdout);

				jrb_insert_int(inodes, stat_buf.st_ino, JNULL);

				if (S_ISDIR(stat_buf.st_mode)) {	
					/* If file is a directory, add it to the recursive call list */
					dll_append(directories, new_jval_s(strdup(s)));
				}
				else {
					/* Print file size and contents of file */
					fd = fopen(s, "r");
					if (fd == NULL) {
						perror(s);
						exit(EXIT_FAILURE);
					}
					*file_sz = stat_buf.st_size;
					fwrite(file_sz, sizeof(off_t), 1, stdout);

					file_buf = (char *) malloc (*file_sz);	

					num_read = fread(file_buf, sizeof(char), *file_sz, fd);
					if (num_read != *((int *)file_sz)) {
						perror("fread()");
						exit(EXIT_FAILURE);
					}

					fwrite(file_buf, sizeof(char), num_read, stdout);

					free(file_buf);
					fclose(fd);
				}
			}
		}
		free(s);
	}

	closedir(d);
	dll_traverse(tmp, directories) {
		traverse_dir(tmp->val.s, inodes, 0, first_dir);
		free(tmp->val.s);
	}

	free_dllist(directories);
	free(inode);
	free(mode);
	free(mod_time);
	free(file_sz);
}

int main(int argc, char* argv[]) {
	
	JRB inodes;
	int first_dir = -1;

	if (argc != 2) {
        fprintf(stderr, "usage: ./tarc directory > directory.tarc\n");
        exit(EXIT_FAILURE);
    }

	(void)remove_prefix(argv[1], argv[1], &first_dir);

	inodes = make_jrb();

	traverse_dir(argv[1], inodes, 1, &first_dir);

	jrb_free_tree(inodes);

	return 0;
}
