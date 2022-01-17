/*  Caroline Rinks
 *  Lab 3 (Fakemake)
 *  CS360 Fall 2021
 *  
 *  This lab creates a program "fakemake" that is a restricted version of make(1). 
 *  Like make, fakemake helps you automate compiling, but unlike make, fakemake
 *  limits itself to making one executable and assumes that you are using gcc 
 *  to do your compilation.
*/

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "Fmakefile.h"

int main (int argc, char * argv[]) {
	
	Fmakefile f;
	IS is;
	int remade, ex_given;
	long int time;
	struct stat ex_file;

	f = new_fmakefile();

	/* Set description file */
	if (argc == 1) {
		is = new_inputstruct("fmakefile");
		if (is == NULL) {
			perror("fmakefile");
			exit(EXIT_FAILURE);
		}
	}
	else if (argc == 2) {
		is = new_inputstruct(argv[1]);
		if (is == NULL) {
			perror(argv[1]);
			exit(EXIT_FAILURE);
		}
	}
	else {
		fprintf(stderr, "usage: ./fakemake filename\n");
        exit(EXIT_FAILURE);
	}

	ex_given = 0;

	/* Read file */
	while (get_line(is) >= 0) {
		if (is->text1[0] == '\n'){
			continue;
		}

		if (is->text1[0] == 'C'){
			add_files(is, f->Cfiles);
		}
		else if (is->text1[0] == 'H') { 
			add_files(is, f->Hfiles);
		}
		else if (is->text1[0] == 'L') {
            add_files(is, f->Lfiles);
        }
		else if (is->text1[0] == 'F') {
            add_files(is, f->Flags);
        }
		else if (is->text1[0] == 'E') {
			if (ex_given == 1) {
				fprintf(stderr, "fmakefile (%d) cannot have more than one E line\n", is->line);
				exit(EXIT_FAILURE);
			}
			ex_given = 1;
			f->ex_name = read_Ename(is);
		}
		else {
			fprintf(stderr, "fakemake (%d): Lines must start with C, H, E, F or L\n", is->line);
			exit(EXIT_FAILURE);
		}
	}

	jettison_inputstruct(is);	// frees memory alloc'd by new_inputstruct()

	if (ex_given == 0) {
		fprintf(stderr, "No executable specified\n");
		exit(EXIT_FAILURE);
	}

	/* Determine if a source file needs to be compiled or executable needs to be made */
	time = process_Hfiles(f->Hfiles);	

	remade = 0;
	time = process_Cfiles(&remade, f, time);

	if (stat(f->ex_name, &ex_file) == -1) {		// if executable does not exist
		compile_str('o', f->ex_name, f);
	}
	else if (remade == 1) {						// if files were recompiled
		compile_str('o', f->ex_name, f);
	}
	else if (time > ex_file.st_mtime) {			// if .o file is more recent than executable
		compile_str('o', f->ex_name, f);
	}
	else{
		printf("%s up to date\n", f->ex_name);
	}
	
	free_fmakefile(f);

	return 0;
}

