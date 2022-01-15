/* Caroline Rinks
 * 13 Sept 2021
 * Lab 2
 *
 * l2p3 does the same thing as l2p2 but uses buffering to read the file "converted". 
 * After reading in the file, it prompts the user to enter a machine name, and then 
 * prints out the IP address and all names associated with that machine.
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "jrb.h"
#include "dllist.h"
#include "jval.h"

typedef struct {
	unsigned char address[4];
	int list_freed;
	int visited;
	Dllist host_names;
} IP;

int main() {

	int fd;
	int i, j, k, count;
	int num_names, size, bytes, bytes_read;
	IP * ip;
	JRB hosts, node;				/* Red-Black tree: key=name, value=ip ptr list */
	Dllist ips, tmp1, tmp2;

	unsigned char * buffer;	
	unsigned char * name, * loc_name;
	unsigned char temp[BUFSIZ];		
	unsigned char * ch_ptr;
	unsigned char c;

	hosts = make_jrb();

	/* Read file "converted" */
	fd = open("converted", O_RDONLY);
	if (fd == -1) {
		perror("Error opening file\n");
	}
	buffer = (unsigned char *) calloc (350000, sizeof(char));
	bytes_read = read(fd, buffer, 350000);

	bytes = 0;
	i = 0;
	count = 0;

	while (count < bytes_read) {
		if (bytes + i >= 350000){
			break;
		}
		i = 0;

		/* Read ip address into new ip struct */
		ip = (IP *) malloc (sizeof(IP));

		ip->host_names = new_dllist();
		ip->address[0] = buffer[bytes+(i++)];
		ip->address[1] = buffer[bytes+(i++)];
		ip->address[2] = buffer[bytes+(i++)];
		ip->address[3] = buffer[bytes+(i++)];
		count += 4;
		
		ip->list_freed = 0;
		ip->visited = 0;

		/* Construct int representing number of names */
		num_names = 0;
		num_names = num_names | buffer[bytes+(i++)] | buffer[bytes+(i++)] | buffer[bytes+(i++)] | buffer[bytes+(i++)];
		count += 4;

		/* Construct each name string by reading in each character */
		for (k = 0; k < num_names; k++){
			j = 0;
			do {
				count += 1;
				c = buffer[bytes+(i++)];
				temp[j] = c;
				++j;
			} while (c != '\0');

			if (count > bytes_read){
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
		bytes += i;
	}

	free(buffer);

	if (close(fd) < 0) {
		perror("Error closing file\n");
		exit(1);
	}

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

	/* Free up memory */
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
