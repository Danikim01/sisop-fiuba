#define _DEFAULT_SOURCE
#define FIRST_FIT  // TODO: Comment it, to run one or the other just use flags
//#define BETS_FIT

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>

#include "malloc.h"
#include "printfmt.h"

#define ALIGN4(s) (((((s) -1) >> 2) << 2) + 4)
#define REGION2PTR(r) ((r) + 1) 
#define PTR2REGION(ptr) ((struct region *) (ptr) -1)

#define MIN_BLOCK_SIZE 16384  //in bytes === 16 Kib
#define MIN_SIZE_TO_RETURN 256 //in bytes, defined in the tp

struct region *region_free_list = NULL;

// |header | region1 | header | region2 |
// |          free               !free  |


int amount_of_mallocs = 0;
int amount_of_frees = 0;
int requested_memory = 0;

// finds the next free region
// that holds the requested size
//
static struct region *
find_free_region(size_t size)
{
	struct region *next = region_free_list;  // Should never be NULL

#ifdef FIRST_FIT

	struct region *prev = NULL;
	while (next) {
		if (next->size >= size && next->free == true) {
			next->free = false;
			// if (prev != NULL) {
			// 	// prev->next = next->next; // A->B->C =>
			// first_fit = B => A->C (A->next = B->next) } else {
			// 	// region_free_list = next->next; //Actualize
			// first node of the free list
			// }
			return next;
		}
		prev = next;
		next = next->next;
	}

#endif

#ifdef BEST_FIT
	// Your code here for "best fit"
#endif

	// If you get to the end of the free list then you couldn't find the region you needed then
	// next = then so this is equivalent to return NULL
	return next;
}


void
actualize_free_region_list(struct region *new_region)
{  // If first region is not initialized for amount of mallocs = 0 then
	// this code should add if act == NULL: region_free_list = new_region
	struct region *act = region_free_list;
	struct region *prev = NULL;
	while (act != NULL) {
		prev = act;
		act = act->next;
		if (act == NULL) {
			prev->next = new_region;
		}
	}
}


static struct region *
grow_heap(size_t size)
{  // If (no hay lo q quiere): crea un bloque del tam que quiere
	struct region *new_region = mmap(NULL,
	                                 size,
	                                 PROT_READ | PROT_WRITE,
	                                 MAP_PRIVATE | MAP_ANONYMOUS,
	                                 -1,
	                                 0);

	if (new_region == MAP_FAILED)
		return NULL;

	new_region->free = true;
	new_region->next = NULL;
	new_region->size = size - sizeof(struct region);
	if (new_region->size <= 0)  // Shoudn't happen, but just in case
		return NULL;

	actualize_free_region_list(
	        new_region);  // If free_region_list = NULL this does nothing

	return new_region;
}

static struct region *
split_free_regions(struct region *region_to_split, size_t desired_size)
{
	//TODO: Check if it makes sense to split if the desired size is big compared to
	//the region. If if have 100 bytes and they ask me for 99, does it make sense
	//to split? 
	
	// Check if the region_to_split can be split
	if (region_to_split->size > desired_size) {
		size_t prev_region_size = region_to_split->size;

		// Update the size of the region to split
		region_to_split->size = desired_size;

		// Obtain the new header position:
		// Move to the region pointer
		void *ptr_to_region = REGION2PTR(region_to_split);
		// Move the desired amount of bytes into the region, plus the
		// size of a struct region (for the new header)
		void *ptr_to_new_header = (char *) ptr_to_region + desired_size;
		struct region *new_header = (struct region *) ptr_to_new_header;

		// Initialize the new header with the remaining size, and update the links in the free list
		new_header->size =
		        prev_region_size - desired_size - sizeof(struct region);
		new_header->next = region_to_split->next;
		new_header->free = true;

		region_to_split->next = new_header;
	}

	// If the region can't be split, just return the original region
	return region_to_split;
}


void coalesce_regions(struct region* curr) {
	struct region* prev = NULL;
	struct region* act = region_free_list;
	struct region* sig = region_free_list->next;

	while (act != NULL) {
		if (act == curr) { //Found my current header
			if (sig != NULL) { // Caso curr = last element
				if (sig->free == true) {
					act->size = act->size + sizeof(struct region) + sig->size;
					act->next = sig->next; //Actualize the linked list
				}
			}

			if (prev != NULL) { //Caso curr = first element
				if (prev->free == true) {
					prev->size = prev->size + sizeof(struct region) + act->size;
					prev->next = act->next; //Actualize the linked list
					if (act == region_free_list) { // Update head if necessary
                        region_free_list = prev;
                    }
				}
			}
		}

		prev = act;
		act = act->next; 
		if (act != NULL) {
			sig = act->next; //To avoid getting a seg fault
		} else {
			sig = NULL;
		}
	}
}


//TODO: When finished, delete it
void print_all_free_list_elements() {
	//printfmt("#########\nFREE:\n##########");
	//printfmt("#########\nMALLOC:\n##########");
	struct region* act = region_free_list;
	int contador = 0;
	while (act != NULL){ 
		printfmt("\n------REGION ACTUAL NUMERO: %d------\n", contador);
		printfmt("TAM: %d\n", act->size + sizeof(struct region));
		printfmt("LIBRE: %d\n", act->free);

		act = act->next;
		contador++;
	}
}


/// Public API of malloc library ///


void *
malloc(size_t size)
{
	struct region *next;

	// aligns to multiple of 4 bytes
	size = ALIGN4(size);

	if (size < MIN_SIZE_TO_RETURN) size = MIN_SIZE_TO_RETURN; 

	if (amount_of_mallocs == 0) {
		// If no first block, we create a bloque of min
		// size (16kib)
		region_free_list = grow_heap(MIN_BLOCK_SIZE);
	}
	// updates statistics
	amount_of_mallocs++;
	requested_memory += size;

	next = find_free_region(size);

	if (!next) {
		// next = grow_heap(size);
		return NULL;  // Primera parte
	}

	next = split_free_regions(next, size);

	next->free = false;  // Before returning the region, mark it as not free
	return REGION2PTR(next);
}

void
free(void *ptr)
{
	// updates statistics
	amount_of_frees++;

	struct region *curr = PTR2REGION(ptr); 
	assert(curr->free == false);

	curr->free = true; 

	coalesce_regions(curr); 
}

void *
calloc(size_t nmemb, size_t size)
{
	// Your code here

	return NULL;
}

void *
realloc(void *ptr, size_t size)
{
	// Your code here

	return NULL;
}

void
get_stats(struct malloc_stats *stats)
{
	stats->mallocs = amount_of_mallocs;
	stats->frees = amount_of_frees;
	stats->requested_memory = requested_memory;
}