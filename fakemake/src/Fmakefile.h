/* Caroline Rinks
 * Lab 3 Header File
 * CS360 Fall 2021
 *
 * Contains Definition of fmakefile struct
*/

#ifndef _FMAKEFILE_H_
#define _FMAKEFILE_H_

#include "fields.h"
#include "jval.h"
#include "dllist.h"

typedef struct fmakefile {
	Dllist Cfiles;
	Dllist Hfiles;
	Dllist Lfiles;
	Dllist Flags;
	char * ex_name;
} *Fmakefile;

extern Fmakefile new_fmakefile();
extern void free_fmakefile(Fmakefile);

extern void add_files(IS, Dllist);
extern char *read_Ename(IS);
extern long int process_Hfiles(Dllist);
extern void compile_str(char, const char *, Fmakefile); 
extern long int process_Cfiles(int *, Fmakefile, const long int);

#endif
