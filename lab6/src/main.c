/* Caroline Rinks
 * Lab 7 (Shell)
 * CS360 Fall 2021
 *
 * Implements a primitive shell, "jsh".
 * jsh reads commands from the command line and stores information about each
 * command in a Command struct. The commands are executed in order by 
 * forking a new process using fork() and executing using execvp(). File 
 * redirection and pipes are also implemented.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "fields.h"
#include "dllist.h"
#include "jrb.h"

typedef struct Command {
	char ** args;
	Dllist symbols;
	int size;
	int index;
	int argn;

} Command;

/* @brief: Sets the prompt to 'jsh:' by default or to a user-specified prompt
 * @param[in] argc: The number of command-line arguments
 * @param[in] argv: Points to all c-string command-line arguments
 * @return: The prompt as a c-string.
*/
char *set_prompt(int argc, char **argv) {
	char * prompt;
	
	if (argc == 2) {
		if (strcmp(argv[1], "-") == 0) {
			prompt = "";
		}
		else {
			prompt = argv[1];
		}
	}
	else if (argc == 1) {
		prompt = "jsh: ";
	}
	else {	
		fprintf(stderr, "usage: ./main prompt\n");
		exit(EXIT_FAILURE);
	}
	return prompt;
}	

/* @brief: Parses arguments from command-line into a new arguments array.
 * @param[in] is: The input struct used to read in each command line.
 * @param[out] arg0: The starting index of the command currently being parsed.
 * @param[in] argn: The ending index of the command currently being parsed.
 * @param[out] command: A Command struct object used to store info for each command.
 * @param[out] last_pipe: An int that indicates if the last command has been parsed.
 * @return: None.
*/
char** parse_args(IS is, int * arg0, int argn, Command * command, int * last_pipe) {

    char ** args;
    int i, k = 0, nsize = 0;

    /* Allocate memory for argument array */
	i = *arg0;
	while ((i < is->NF) && (strcmp(is->fields[i], "|") != 0)) {
		++nsize;
		++i;
	}
	nsize = sizeof(char *) * nsize;
    nsize += sizeof(char *);

    args = (char **) malloc(nsize);

	/* Copy in arguments from is->fields[] */
	for (i = *arg0; i < argn; i++) {
		/* remove '>', '<', and '>>' from args and save in a queue for later */
		if (strcmp(is->fields[i], ">") == 0) {
            dll_append(command->symbols, new_jval_s(">"));
            is->fields[i] = (char *)NULL;
			args[k] = (char *)NULL;
			++k;
			continue;
		}
		else if (strcmp(is->fields[i], "<") == 0) {
            dll_append(command->symbols, new_jval_s("<"));
            is->fields[i] = (char *)NULL;
			args[k] = (char *)NULL;
			++k;
			continue;
        }
		else if (strcmp(is->fields[i], ">>") == 0) {
            dll_append(command->symbols, new_jval_s(">>"));
            is->fields[i] = (char *)NULL;
			args[k] = (char *)NULL;
			++k;
			continue;
        }
		else if (strcmp(is->fields[i], "|") == 0) {
            is->fields[i] = (char *)NULL;
			args[k] = (char *)NULL;
			*arg0 = i + 1;		// update starting index for next command
			return args;
		}	
		args[k] = strdup(is->fields[i]);	
		++k;
	}	

	args[k] = (char *)NULL;
	*arg0 = is->NF;
	*last_pipe = 1;

    return args;
}

/* @brief: Implements file redirection if specified by the user by extracting file
 *         redirection symbols from a queue.
 * @param[in] is: An inputstruct used to read in each command line.
 * @param[in] command: A Command struct object holding info about the current command.
 * @return: None.
*/
void set_io(IS is, Command * command) {
	int fd1, fd2, fd3, i;

	for (i = command->index; i < command->argn; i++) {
		if (is->fields[i] == NULL) {
			if (strcmp(dll_first(command->symbols)->val.s, "<") == 0) {
				/* Open File for reading */
				fd1 = open(is->fields[i+1], O_RDONLY);
				
				if (fd1 < 0) {
					perror(is->fields[i+1]);
					exit(1);
				}
				if (dup2(fd1, 0) != 0) {
					perror("dup2(f1, 0)");
					exit(1);
				}
				close(fd1);

				dll_delete_node(dll_first(command->symbols));
			}
			else if (strcmp(dll_first(command->symbols)->val.s, ">") == 0) {
				/* Open File for writing */
				fd2 = open(is->fields[i+1], O_WRONLY | O_TRUNC | O_CREAT, 0644);
				
				if (fd2 < 0) {
					perror(is->fields[i+1]);
					exit(1);
				}
				if (dup2(fd2, 1) != 1) {
					perror("dup2(f2, 1)");
					exit(1);
				}
				close(fd2);

                dll_delete_node(dll_first(command->symbols));
			}
			else if (strcmp(dll_first(command->symbols)->val.s, ">>") == 0) {
				/* Open File for Appending */
				fd3 = open(is->fields[i+1], O_WRONLY | O_APPEND | O_CREAT, 0644);
				
				if (fd3 < 0) {
					perror(is->fields[i+1]);
					exit(1);
				}
				if (dup2(fd3, 1) != 1) {
					perror("dup2(f3, 1)");
					exit(1);
				}
				close(fd3);

                dll_delete_node(dll_first(command->symbols));
			}
		}
	}
}

/* @brief: Creates a queue of commands specified by the user to execute.
 * @param[in] is: The input struct used to read in commands.
 * @return: A pointer to the head of the newly created command queue.
 */
Dllist new_command_queue(IS is) {
	
	Dllist command_list;
	Command * command;
	int arg0 = 0, last_pipe = 0;

	command_list = new_dllist();
	
	/* Store info for each command in a Command struct object,
	 * and store each object in a Double-Linked List */
	while (last_pipe == 0) {
		command = (Command *) malloc(sizeof(struct Command));
		command->index = arg0;
		command->symbols = new_dllist();
		command->args = parse_args(is, &arg0, is->NF, command, &last_pipe);
		command->size = arg0 - command->index;
		command->argn = arg0 - 1;

		dll_append(command_list, new_jval_v((void *)command));
	}
	return command_list;
}

/* @brief: Frees memory alloc'd for each Command.
 * @param[in] command: The Commmand struct object to free.
 * @return: None.
*/
void free_command(Command * command) {
	for (int i = 0; i <= (command->argn - command->index); i++) {
		free(command->args[i]);
	}
	free(command->args);
	free_dllist(command->symbols);
	free(command);
}

/* @brief: Frees any memory allocated for lists or trees.
 * @param[in] is_list: A red-black tree of input structs used to read commands in exec_commands().
 * @param[in] processes: A red-black tree that stores the ids of processes.
 * @param[in] command_list: A double-linked list used to store a queue of commands.
 * @return: None.
*/
void free_lists(JRB is_list, JRB processes, Dllist command_list) {
    JRB tmp;

    jrb_traverse(tmp, is_list) {
        jettison_inputstruct((IS)tmp->val.v);
    }
    jrb_free_tree(is_list);
    jrb_free_tree(processes);
	free_dllist(command_list);
}

/* @brief: Reads from the command line and executes each command by creating a new process 
 *		with fork(). A red-black tree is used to keep store child processes.
 * @param[in] prompt: The prompt to be used for the shell.
 * @param[in] is: An inputstruct used to read a command from stdin
 * @return None.
*/
void exec_commands(char * prompt, IS is) {

	int status, pid, child; 
	int last_pipe, first_pipe, prev_fd;
	int pipefd[2];
	Dllist command_list;
	JRB processes, is_list;
	Command * command;

	processes = make_jrb();
	is_list = make_jrb();
	(void) jrb_insert_int(is_list, getpid(), new_jval_v((void *) is));

	/* Command-Line Loop */
	printf("%s", prompt);
	while(get_line(is) >= 0) {

		/* Check command isn't blank */
		if (is->NF == 0) {
			printf("%s", prompt);
			continue;
		}

		/* Quit Program if user types 'exit' */
		if (strcmp(is->fields[0], "exit") == 0) {
			exit(0);
		}

		/* Store each command from command-line in a Queue */
		command_list = new_command_queue(is);
		last_pipe = 0;
		first_pipe = 1;
		prev_fd = -1;

		/* Execute each command by forking off a new process,
		 * and set up communication with other processes via pipe */
		while (last_pipe == 0) {
			command = (Command *)(dll_first(command_list)->val.v);
			dll_delete_node(dll_first(command_list));
		
			if (dll_empty(command_list)) {
				last_pipe = 1;
			}			
				
			if (pipe(pipefd) < 0) {
				perror("pipe");
				exit(1);
			}
			
			pid = fork();
			if (pid == 0) {	
				(void) jrb_insert_int(processes, getpid(), new_jval_i(getppid()));

				/* Set up file redirection if specified */
				set_io(is, command);

				/* Set up pipe according to command's position (first, middle, or last) */
				if (first_pipe == 1) {
					close(pipefd[0]);
					if (last_pipe == 0) {
						if (dup2(pipefd[1], 1) != 1) {
							perror("dup2(pipefd[1], 1)");
							exit(1);
						}
					}
					
					/* If only a single command was specified, close pipe */
					if (last_pipe == 1) {
						close(pipefd[1]);
					}
				}
				else if (last_pipe == 1) {
					close(pipefd[1]);
					close(pipefd[0]);
					if (dup2(prev_fd, 0) != 0) {
						perror("dup2(prev_fd, 0)");
						exit(1);
					}
					close(prev_fd);
				}
				else {
					if (dup2(pipefd[1], 1) != 1) {
						perror("dup2(pipefd[1], 1)");
						exit(1);
					}
					close(pipefd[0]);
					if (dup2(prev_fd, 0) != 0) {
						perror("dup2(prev_fd, 0)");
						exit(1);
					}
				}
				
				/* Remove '&' from command if it was specified */
				if (last_pipe == 1) {
					if (strcmp(is->fields[is->NF-1], "&") == 0) {
						command->args[command->argn-command->index] = (char *) NULL;
						is->NF -= 1;
					}
				}

				/* Execute command. Only returns if an error occured */
				execvp(command->args[0], command->args);
				perror(command->args[0]);
				exit(1);
			}
			else { 
				(void) jrb_insert_int(processes, pid, new_jval_i(getpid()));

				close(pipefd[1]);
				if (prev_fd != -1)	{
					close(prev_fd);
				}
				if (last_pipe == 1) {
					close(pipefd[0]);
				}
				prev_fd = pipefd[0];
		
				/* Wait if '&' was not specified */	
				if (strcmp(is->fields[is->NF-1], "&") != 0) {
					
					while (!jrb_empty(processes)) {
						child = wait(&status);
						if (child < 0) {
							perror("wait");
							exit(1);
						}
						jrb_delete_node(jrb_find_int(processes, child));
					}
				}
				first_pipe = 0;
			}
			free_command(command);
		}

		/* return prompt when done */
		printf("%s", prompt);

		is = new_inputstruct(NULL);
		(void) jrb_insert_int(is_list, getpid(), new_jval_v((void *) is));
	}
	free_lists(is_list, processes, command_list);
}

int main(int argc, char* argv[]) {

    char * prompt;
    IS is;

	prompt = set_prompt(argc, argv);

	is = new_inputstruct(NULL);
	if (is == NULL) {
		perror("main.c:");
		return -1;
	}

	exec_commands(prompt, is); 
	
	return 0;
}
