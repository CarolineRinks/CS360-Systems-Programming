/*	Caroline Rinks
 *	Lab 3 Source file
 *	CS360 Fall 2021
 *	
 *	Contains Implementation of functions in Fmakefile.h
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "Fmakefile.h"

/* @brief: Creates a new fmakefile struct.
 * @return: The newly created fmakefile struct.
*/
Fmakefile new_fmakefile(){
	Fmakefile f;
	
	f = (Fmakefile) malloc (sizeof(struct fmakefile));
	
	f->Cfiles = new_dllist();
	f->Hfiles = new_dllist();
	f->Lfiles = new_dllist();
	f->Flags = new_dllist();

	f->ex_name = NULL;

	return f;
}

/* @brief: Frees all allocated memory for given fmakefile struct.
 * @param[in] f: The fmakefile struct to free.
 * @return: none
*/
void free_fmakefile(Fmakefile f){
	Dllist tmp;
	
	dll_traverse(tmp, f->Cfiles){
        free(tmp->val.s);
    }
    dll_traverse(tmp, f->Hfiles){
        free(tmp->val.s);
    }
    dll_traverse(tmp, f->Lfiles){
        free(tmp->val.s);
    }
    dll_traverse(tmp, f->Flags){
        free(tmp->val.s);
    }

    free(f->ex_name);
    free_dllist(f->Cfiles);
    free_dllist(f->Hfiles);
    free_dllist(f->Lfiles);
    free_dllist(f->Flags);
	free(f);
}

/* @brief: Reads filenames from an IS and appends them to a given Dllist.
 * @param[in] is: The input struct to read filenames from.
 * @param[out] files: The double-linked list to append filenames to.
 * @return: none
*/
void add_files(IS is, Dllist files){
	for (int i = 1; i < is->NF; i++){
		if (is->text1[0] == 'C'){
			if (strstr(is->fields[i], ".c") == NULL) {
				fprintf(stderr, "fakemake: C file %s needs to end with .c\n", is->fields[i]);
				exit(EXIT_FAILURE);
			}
		}
		else if (is->text1[0] == 'H'){
			if (strstr(is->fields[i], ".h") == NULL) {
				fprintf(stderr, "fakemake: H file %s needs to end with .h\n", is->fields[i]);
                exit(EXIT_FAILURE);
			}
		}
		dll_append(files, new_jval_s(strdup(is->fields[i])));
	}
}	

/* @brief: Constructs a string for the executable name.
 * @param[in] is: The input struct to read the executable name from.
 * @return: The executable name string.
*/
char *read_Ename(IS is){
	int size, n;
	char * ex_name;

	if (is->NF > 1){
		size = strlen(is->fields[1]);
		ex_name = (char *) malloc(sizeof(char) * size + 1);
		n = sprintf(ex_name, is->fields[1]);
		if (n == 0){
			fprintf(stderr, "fakemake: Problem reading E file %s\n", is->fields[1]);
			exit(EXIT_FAILURE);
		}
	}
	else{
		fprintf(stderr, "No executable specified\n");
		exit(EXIT_FAILURE);
	}

	return ex_name;
}

/* @brief: Given a list of header files, determines the maximum (most recent) time stat of header files overall.
 * @param[in] Hfiles: The list of header files to check the times of.
 * @return: The maximum time found.
*/
long int process_Hfiles(Dllist Hfiles){
	
	struct stat buf;
	Dllist tmp;
	long int max_time;
	
	max_time = 0;

	dll_traverse(tmp, Hfiles) {
		if (stat(tmp->val.s, &buf) == -1) {
			perror(tmp->val.s);
			exit(EXIT_FAILURE);
		}
		if (buf.st_mtime > max_time) {
			max_time = buf.st_mtime;
		}
	}
	return max_time;
}

/* @brief: Builds a compile string and calls system() to execute the string as a command.
 * @param[in] mode: Specifies "gcc -c" or "gcc -o".
 * @param[in] file: The executable if mode 'o' or the source file if mode 'c'.
 * @param[in] Cfiles: The list of source files to compile.
 * @param[in] Lfiles: The list of libraries to include when compiling.
 * @param[in] Flags: The list of flags to include when compiling.
 * @return none
*/
void compile_str(char mode, const char * file, Fmakefile f) {

	Dllist tmp;
	int size, val;
	char * tmp_str;

	/* Determine size of compile string */
	size = strlen("gcc -c ");
	if (mode == 'o') {
		dll_traverse(tmp, f->Cfiles) {
			size += strlen(tmp->val.s) + 1;
		}
	}
	size += strlen(file);

	dll_traverse(tmp, f->Flags) {
		size += strlen(tmp->val.s) + 1;
	}
	dll_traverse(tmp, f->Lfiles) {
		size += strlen(tmp->val.s) + 1;
	}

	/* Construct compile string */
	tmp_str = (char *) malloc (sizeof(char) * size + 1);

	if (mode == 'c') {
		sprintf(tmp_str, "gcc -c");
	}
	else if (mode == 'o') {
		sprintf(tmp_str, "gcc -o ");
		strcat(tmp_str, file);
	}
	else {
		fprintf(stderr, "invalid mode given\n");
		exit(EXIT_FAILURE);
	}

	dll_traverse(tmp, f->Flags) {
		strcat(tmp_str, " ");
		strcat(tmp_str, tmp->val.s);
	}
	if (mode == 'o') {
		dll_traverse(tmp, f->Cfiles) {
			strcat(tmp_str, " ");
			strcat(tmp_str, tmp->val.s);
			tmp_str[strlen(tmp_str)-1] = 'o';
		}
		dll_traverse(tmp, f->Lfiles) {
            strcat(tmp_str, " ");
            strcat(tmp_str, tmp->val.s);
        }
	}
	else {
		strcat(tmp_str, " ");
		strcat(tmp_str, file);
	}

	/* Print and execute compile string */
	printf("%s\n", tmp_str);
	
	val = system(tmp_str);
	if (val == -1) {
		perror(tmp_str);
		exit(EXIT_FAILURE);
	}
	else if (val == 256) {
		fprintf(stderr, "Command failed.  Exiting\n");
		exit(EXIT_FAILURE);
	}
	free(tmp_str);
}

/* @brief: Given a list of source files, determines whether each file should be compiled.
 * @param[in] Cfiles: The list of source files.
 * @param[in] Lfiles: A list of libraries to include when compiling. 
 * @param[in] Flags: A list of flags to include when compiling. 
 * @param[in] H_time: The maximum time found for header files. 
 * @param[out] remade: Indicates whether any files were recompiled (0=no|1=yes).
 * @return: The maximum time found for source files.
*/
long int process_Cfiles(int * remade, Fmakefile f, const long int H_time){
	
	int size;
	long int max_otime;
	Dllist tmp;
	struct stat c_buf, o_buf;
	char * o_file;
	char msg_buf[100];

	max_otime = 0;

	dll_traverse(tmp, f->Cfiles) {
		o_file = (char *) malloc (sizeof(char) * strlen(tmp->val.s) + 1);
		sprintf(o_file, tmp->val.s);
		
		size = strlen(o_file);
		o_file[size-1] = 'o';

		if (stat(tmp->val.s, &c_buf) < 0) {		// if .c file doesn't exit
			sprintf(msg_buf, "fmakefile: %s", tmp->val.s);			
			perror(msg_buf);
			exit(EXIT_FAILURE);
        }

		if (stat(o_file, &o_buf) < 0) {		// if .o file doesn't exist for .c file
			compile_str('c', tmp->val.s, f); 
			*remade = 1;
		}
		else {
			if (c_buf.st_mtime > o_buf.st_mtime) {		// if .c file is more recent than .o file
				compile_str('c', tmp->val.s, f);
				*remade = 1;
			}
			else if (H_time > o_buf.st_mtime) {		// if .h file is more recent than .o file
				compile_str('c', tmp->val.s, f);
				*remade = 1;
			}

			if (o_buf.st_mtime > max_otime) {
				max_otime = o_buf.st_mtime;
			}
		}
		free(o_file);
	}
	
	return max_otime;
}

