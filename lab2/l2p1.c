/* Caroline Rinks
 * Lab 2 (Buffering)
 * CS360 Fall 2021
 *
 * l2p1 reads host information from the file "converted" using buffered std I/O routines
 * and stores the info in a red-black tree keyed on machine names. After reading in the 
 * file, the user is prompted to enter a machine name. The IP address and all names
 * associated with that machine are printed.
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "jrb.h"
#include "dllist.h"
#include "jval.h"

typedef struct {
	unsigned char address[4];
	int visited;
	int list_freed;
	Dllist host_names;
} IP;

int main() {
	int created = 0;
	int freed = 0;

	FILE * pfile;					
	int i, j, num_names, size, skip;
	IP * ip;
	JRB hosts, node;				/* Red-Black tree: key=name, value=IP list */
	Dllist ips, tmp1, tmp2;

	unsigned char * buffer;	
	unsigned char * name, * loc_name;
	unsigned char temp[BUFSIZ];		
	unsigned char * ch_ptr;
	unsigned char c;

	hosts = make_jrb();

	/* Read from file converted */	
	pfile = fopen("converted", "r");
	if (pfile == NULL){
		perror("Error opening file\n");
	}

	buffer = (unsigned char *) malloc (sizeof(unsigned char)*4);

	while (!feof(pfile)) {
		/* Read ip address into new ip struct */
		fread(buffer, sizeof(int), 1, pfile);
		ip = (IP *) malloc (sizeof(IP));

		ip->host_names = new_dllist();
		ip->address[0] = buffer[0];
		ip->address[1] = buffer[1];
		ip->address[2] = buffer[2];
		ip->address[3] = buffer[3];
		ip->list_freed = 0;
		ip->visited = 0;

		/* Read number of names */
		fread(buffer, sizeof(int), 1, pfile);
		
		/* Construct int representing number of names */
		num_names = 0;
		num_names = num_names | buffer[0] | buffer[1] | buffer[2] | buffer[3];
		
		/* Construct each name string by reading in each character*/
		for (i = 0; i < num_names; i++){
			j = 0;
			while (1) {
				c = fgetc(pfile);
				if (c == '\0' || feof(pfile)) {
					temp[j] = c;
					break;
				}
				temp[j] = c;
				++j;
			}			
			if (feof(pfile)){
				break;
			}

			/* Construct name string */
			size = strlen(temp) + 1;
			name = (unsigned char *) calloc(size, sizeof(char));
			strncpy(name, temp, size);
		
			/* Extract local name from absolute name if needed */
            ch_ptr = strchr(name, '.');
            if (ch_ptr != NULL) {
                size = ch_ptr - name;
				loc_name = (unsigned char *) calloc(size+1, sizeof(char));
				strncpy(loc_name, name, size);

                dll_append(ip->host_names, new_jval_s(loc_name));

                node = jrb_find_str(hosts, loc_name);
                if (node == NULL){
                    ips = new_dllist();
					dll_append(ips, new_jval_v((void *) ip));
					node = jrb_insert_str(hosts, loc_name, new_jval_v((void *) ips));
                }
                else{
					dll_append(((Dllist)node->val.v), new_jval_v((void *) ip));
                }
            }

			dll_append(ip->host_names, new_jval_s(name));

			node = jrb_find_str(hosts, name);
			if (node == NULL){
				ips = new_dllist();
				dll_append(ips, new_jval_v((void *) ip));
				node = jrb_insert_str(hosts, name, new_jval_v((void *) ips));
			}
			else{
				dll_append(((Dllist)node->val.v), new_jval_v((void *) ip));
			}
		}
	}

	free_dllist(ip->host_names);
	free(ip);
	free(buffer);

	fclose(pfile);
	printf("Hosts all read in\n");

	printf("\nEnter host name: ");	
	while(scanf("%s", temp) > 0){

		size = strlen(temp) + 1;
		name = (unsigned char *) malloc(sizeof(char) * size);
		strncpy(name, temp, size);

		node = jrb_find_str(hosts, name);
		if (node == NULL) {
			printf("no key %s\n\n", name);
		}
		else{
			dll_traverse(tmp1, ((Dllist)node->val.v)){
				ip = (IP *)tmp1->val.v;
				printf("%d.%d.%d.%d: ", ip->address[0], ip->address[1], ip->address[2], ip->address[3]);
				dll_traverse(tmp2, ip->host_names) {
					printf(" %s", jval_s(tmp2->val));
				}
				printf("\n\n");
			}
		}
		free(name);
		printf("Enter host name: ");
	}
	printf("\n");

	/* Free allocated memory */
	jrb_traverse(node, hosts){
        ips = (Dllist)node->val.v;
        dll_traverse(tmp1, ips){
            ip = (IP *)tmp1->val.v;
            ip->visited += 1;
			if (ip->list_freed == 0){
                dll_traverse(tmp2, ip->host_names){
                    free(tmp2->val.s);
                }
                ip->list_freed = 1;
                free_dllist(ip->host_names);
            }
        }
    }	
	jrb_traverse(node, hosts){
		ips = (Dllist)node->val.v;
		dll_traverse(tmp1, ips){
			ip = (IP *)tmp1->val.v;
			ip->visited -= 1;
			if (ip->visited == 0){
				free(ip);
			}
		}
		free_dllist(ips);
	}
	jrb_free_tree(hosts);

	return 0;
}
