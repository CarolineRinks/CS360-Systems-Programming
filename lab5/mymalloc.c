/* Caroline Rinks
 * Lab 6 (malloc)
 * CS360 Fall 2021
*/ 

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "mymalloc.h"

struct block {
	unsigned int size;
    struct block* flink;
};

struct block *head = NULL;

/* @brief: Allocates size amount of memory by removing it from the free list. 
 * @param[in] size: The amount of memory requested for allocation.
 * @return: The address of allocated memory.
*/
void* my_malloc(size_t size) {
	struct block * free_ptr, * node, * prev_node;
	void * mem;
	int * alloc_ptr;
	int prev_size;

	if (size <= 0){
		fprintf(stderr, "invalid size given to my_malloc\n");
		exit(EXIT_FAILURE);
	}

	/* Account for padding and book-keeping bytes */
	int padding = size % 8;
	if (padding != 0) {
		size = size + (8 - padding);
	}
	size += 8;

	free_ptr = (struct block *)free_list_begin();

	/* Check free list for free memory */
	while (free_ptr != NULL) {
		if (free_ptr->size >= size) break;

		free_ptr = free_list_next(free_ptr);
	}

	/* Call sbrk() to get more memory if free list is empty */
	if (free_ptr == NULL) {
		if (size < 8192) {
			mem = sbrk(8192);
			free_ptr = (struct block *)mem;
			free_ptr->size = 8192;
		}
		else {
			mem = sbrk(size);
			free_ptr = (struct block *)mem;
			free_ptr->size = size;
		}
		free_ptr->flink = NULL;

		if (head == NULL) {
			head = free_ptr;
		}
		else{
			node = head;
			while (node->flink != NULL) {	// Find last node on free list
				node = node->flink;
			}
			node->flink = free_ptr;
		}
	}

	/* Now have free_ptr, which is either new mem from sbrk or slot of mem on free list */
	prev_size = free_ptr->size;
	prev_node = free_ptr->flink;

	/* Allocate memory for user and update links */
	alloc_ptr = (void *)free_ptr;
	*alloc_ptr = size;

	/* If no free memory is remaining */
	if (prev_size - size == 0) {
		if (head->flink == NULL) {
			head = NULL;
			return (alloc_ptr + 2);
		}
		
		node = head;
		prev_node = head;
		while (node < free_ptr) {
			prev_node = node;
			node = node->flink;
		}
		prev_node->flink = free_ptr->flink;

		return (alloc_ptr + 2);
	}

	/* Return remaining free memory by moving free list ptr and decrementing its size */
	free_ptr = (void *)free_ptr + size;
	free_ptr->size = prev_size - size;
	free_ptr->flink = prev_node;

	/* If malloc'd at current head of free list, update head */
	if (free_list_begin() == alloc_ptr) {
		head = free_ptr;
		return (alloc_ptr + 2);
	}

	/* If malloc'd elsewhere in free list */ 
	node = head;
	prev_node = head;
	while (node < (struct block *)alloc_ptr) {
		prev_node = node;
		node = node->flink;
	}
	prev_node->flink = free_ptr;

	return (alloc_ptr + 2);
}

/* @brief: Frees previously allocated memory by putting a new free list node back on the free list.
 * @param[in] ptr: The address of previously allocated memory to free.
 * @return: none
*/
void my_free(void * ptr) {
	struct block * node, * last_node, * prev_node;
	int * size, * prev;

	size = (int *)ptr - 2;
	node = (struct block *)size;
	node->flink = NULL;

	/* Case: No free memory exists on list */
	if (head == NULL) {
		head = node;
		head->flink = NULL;
		return;
	}
	
	/* Case: New node should be 1st node on free list */
	if (node < head) {
		node->flink = head;
		head = node;
		return;
	}

	/* Case: New node should be last node on free list */
	last_node = head;
	prev_node = head;
	while (last_node->flink != NULL) {
		prev_node = last_node;
		last_node = last_node->flink;
	}
	if (node > last_node) {
		last_node->flink = node;
		node->flink = NULL;
		return;
	}

	/* Case: New node should be somewhere in middle of free list */
	last_node = head;
    prev_node = head;
    while (last_node < node) {
        prev_node = last_node;
        last_node = last_node->flink;
    }
	node->flink = last_node;
	prev_node->flink = node;
}

/* @brief: Returns the 1st node on the free list or NULL if the list is empty.
 * @return: The 1st node on the free list or NULL.
*/
void* free_list_begin() {
	if (head == NULL){
		return NULL;	
	}
	else{
		return head;
	}
}

/* @brief: Returns the next free list node after "node" or NULL if node is the last node.
 * @param[in] node: The free list node whose flink is the return value.
 * @return: The free list node after "node".
*/
void* free_list_next(void * node) {
	return ((struct block *)node)->flink;
}

/* @brief: Combines 2 adjacent free list nodes into a single node on the list.
 * @return: none
*/
void coalesce_free_list() {
	struct block * node, * next;

	node = head;
	while (node->flink != NULL){
		next = node->flink;
		if (next == ((void*)node + node->size)) {
			node->flink = next->flink;
			node->size += next->size;
		}
		else{
			node = node->flink;
			if (node == NULL) {
				break;
			}
		}
	}
}
