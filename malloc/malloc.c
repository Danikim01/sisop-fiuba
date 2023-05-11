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

#define ALIGN4(s) (((((s) -1) >> 2) << 2) + 4) //Get the nearset multiple of 4 of s, this just rounds up
#define REGION2PTR(r) ((r) + 1) 
#define PTR2REGION(ptr) ((struct region *) (ptr) -1)

#define SMALL_BLOCK_SIZE 16384  //in bytes === 16 Kib
#define MEDIUM_BLOCK_SIZE 1048576 //in bytes === 1Mib
#define LARGE_BLOCK_SIZE 33554432  //in bytes === 32Mib

#define MIN_SIZE_TO_RETURN 256 //in bytes, defined in the tp

struct block* small_size_block_list = NULL;
struct block* medium_size_block_list = NULL;
struct block* large_size_block_list = NULL; 

int amount_of_mallocs = 0;
int amount_of_frees = 0;
int requested_memory = 0;

// finds the next free region
// that holds the requested size
//

//Finds the first region within a block that can hold at least size bytes
static struct region* region_first_fist(struct block* block, size_t size) {
	struct region* act = block->first_region;
	while (act != NULL) {
		if (act->size >= size && act->free == true) {
			act->free = false;

			return act;
		}
		act = act->next;
	}

	return NULL;
}


static struct region* first_fit(struct block* block_list, size_t size) {
	struct region* first_fitting_region = NULL;
	while (block_list != NULL) {
		first_fitting_region = region_first_fist(block_list, size);

		if (first_fitting_region != NULL) return first_fitting_region;

		block_list = block_list->next;
	}

	return NULL;
}


static struct region *
find_free_region(size_t size)
{
	struct region* fitting_region = NULL;  // Should never be NULL

#ifdef FIRST_FIT

	if (size <= SMALL_BLOCK_SIZE) {
		fitting_region = first_fit(small_size_block_list, size);
		if (fitting_region != NULL) return fitting_region;

		fitting_region = first_fit(medium_size_block_list, size);
		if (fitting_region != NULL) return fitting_region;

		fitting_region = first_fit(large_size_block_list, size);
	}
	else if (size <= MEDIUM_BLOCK_SIZE) {
		fitting_region = first_fit(medium_size_block_list, size);
		if (fitting_region != NULL) return fitting_region;

		fitting_region = first_fit(large_size_block_list, size);
	} 
	else {
		fitting_region = first_fit(large_size_block_list, size);
	}

#endif

#ifdef BEST_FIT
	// Your code here for "best fit"
#endif

	// If you get to the end of the free list then you couldn't find the region you needed then
	// next = then so this is equivalent to return NULL
	return fitting_region;
}


// void
// actualize_free_region_list(struct region *new_region)
// {  // If first region is not initialized for amount of mallocs = 0 then
// 	// this code should add if act == NULL: region_free_list = new_region
// 	struct region *act = region_free_list;
// 	struct region *prev = NULL;
// 	while (act != NULL) {
// 		prev = act;
// 		act = act->next;
// 		if (act == NULL) {
// 			prev->next = new_region;
// 		}
// 	}
// }


void actualize_block_list(struct block* new_block, struct block** block_list) {	
	if (block_list == NULL) block_list = new_block;

	//The insertion is made in the beginning so this operation is O(1)
	struct block* prev_first_block_in_list = *block_list;
	*block_list = new_block;
	new_block->next = prev_first_block_in_list;
}


static struct region *
grow_heap(size_t block_size)
{  // If (no hay lo q quiere): crea un bloque del tam que quiere

	// |                                                 |
	// | bloque  | header1 | region1 | header2 | region2 | ...

	//TODO: Check if we should ask for block_size - sizeof(block) - sizeof(header) insted
	struct block* new_block = mmap(NULL,
	                                 block_size,
	                                 PROT_READ | PROT_WRITE,
	                                 MAP_PRIVATE | MAP_ANONYMOUS,
	                                 -1,
	                                 0);

	if (new_block== MAP_FAILED)
		return NULL;
	// printfmt("#########\nFREE:\n##########");

	new_block->next = NULL;
	new_block->first_region = REGION2PTR(new_block); //The memory chunck starts where block ends

	new_block->first_region->free = true;
	new_block->first_region->next = NULL;
	new_block->first_region->prev = NULL;
	new_block->first_region->size = block_size - sizeof(struct region) - sizeof(struct block);
	if (new_block->first_region->size <= 0) 
		return NULL;


	switch (block_size) {
		case SMALL_BLOCK_SIZE:
			actualize_block_list(new_block, &small_size_block_list);

			break;
		case MEDIUM_BLOCK_SIZE:
			actualize_block_list(new_block, &medium_size_block_list);

			break;
		case LARGE_BLOCK_SIZE:
			actualize_block_list(new_block, &large_size_block_list);

			break;
	}

	return new_block->first_region;
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
		new_header->prev = region_to_split;

		region_to_split->next = new_header;
	}

	// If the region can't be split, just return the original region
	return region_to_split;
}


void coalesce_regions(struct region* curr) {
	struct region* prev = curr->prev;
	struct region* next = curr->next;

	if (next != NULL) {
		if (next->free == true) {
			curr->size = curr->size +
					    sizeof(struct region) +
					    next->size;
			curr->next = next->next;  // Actualize the linked list
		}
	}

	if (prev != NULL) {
		if (prev->free == true) {
			prev->size = prev->size +
					    sizeof(struct region) +
					    curr->size;
			prev->next = curr->next;  // Actualize the linked list
		}
	}
}


//TODO: When finished, delete it
void print_all_free_list_elements(struct block* list) {
	//printfmt("#########\nFREE:\n##########");
	//printfmt("#########\nMALLOC:\n##########");
	if (list == NULL) {
		printfmt("\n##################ACA NO HABIA NADA################\n");
		return;
	}
	struct region* act = list->first_region;
	int contador = 0;
	while (act != NULL){ 
		printfmt("\n------REGION ACTUAL NUMERO: %d------\n", contador);
		printfmt("TAM: %d\n", act->size + sizeof(struct region));
		printfmt("LIBRE: %d\n", act->free);
		printfmt("TAM DEL BLOCK: %d\n", sizeof(struct block));
		printfmt("TAM DEL REGION: %d\n", sizeof(struct region));

		act = act->next;
		contador++;
	}
}


size_t determine_block_size(size_t size) {
	size_t block_size = 0;
	if (size <= SMALL_BLOCK_SIZE) {
		block_size = SMALL_BLOCK_SIZE;
	} else if (size <= MEDIUM_BLOCK_SIZE) {
		block_size = MEDIUM_BLOCK_SIZE;
	} else {
		block_size = LARGE_BLOCK_SIZE;
	}

	return block_size;
}

size_t validate_size(size_t size) {
	// TODO: Check malloc result if it fails

	// aligns to multiple of 4 bytes
	size_t new_size = ALIGN4(size);

	// TODO: Check malloc result in this case and if i have to have in consideration
	//the header size
	if (new_size > LARGE_BLOCK_SIZE)
		return 0;

	// if the size that's being asked is lower than a minimum, then return the minimum
	if (new_size < MIN_SIZE_TO_RETURN)
		new_size = MIN_SIZE_TO_RETURN;


	return new_size;

}

//If this function fails it returns 0, otherwise 1
// int free_empty_blocks(struct block** desired_block_list, size_t block_size) {
// 	struct block* prev = NULL;
// 	struct block* act = *desired_block_list;
// 	while (act != NULL){
// 		if (act->first_region->size + sizeof(struct block) + sizeof(struct region) == block_size) {
// 			if (prev != NULL) prev->next = act->next; //Actualize the list
// 			else *desired_block_list = act->next;

// 			struct region* top_of_header_ptr = act->first_region - 1;
// 			struct block* top_of_block_ptr = (struct block*) top_of_header_ptr;
// 			top_of_block_ptr = top_of_block_ptr - 1;

// 			void* munmap_ptr = (void*) top_of_block_ptr;

// 			if (munmap(munmap_ptr, block_size) == -1) return 0;
// 		} 
// 		prev = act;
// 		printfmt("PREV MATA\n");
// 		act = act->next;
// 		printfmt("ACT MATA\n");
// 	}


// 	return 1;
// }

/// Public API of malloc library ///


void *
malloc(size_t size)
{
	if (!(size = validate_size(size))) return NULL;
	printfmt("\n##############MALLOC(%d)###########\n", size);

	struct region *next = NULL;

	// updates statistics
	amount_of_mallocs++;
	requested_memory += size;

	next = find_free_region(size);

	if (!next) {
		size_t block_size = determine_block_size(size);
		next = grow_heap(block_size);
	}

	next = split_free_regions(next, size);

	next->free = false;  // Before returning the region, mark it as not free

	print_all_free_list_elements(small_size_block_list);
	// print_all_free_list_elements(medium_size_block_list);

	return REGION2PTR(next);
}

void
free(void *ptr)
{
	printfmt("\n##############FREE###########\n");
	// updates statistics
	amount_of_frees++;

	struct region *curr = PTR2REGION(ptr); 
	assert(curr->free == false);

	curr->free = true; 

	coalesce_regions(curr); 

	// // If there's an empty block, then it should be freed, even if this is O(n) and 
	// // the region could have a reference to the block it belongs to, it's done this way
	// // in order to avoid having circular references (block already has a reference to region)
	// if (curr->size <= SMALL_BLOCK_SIZE) {
	// 	//printfmt("\n##################ACA NO HABIA NADA################\n");
	// 	int free_empty_blocks_success = free_empty_blocks(&small_size_block_list, SMALL_BLOCK_SIZE);
	// 	// printfmt("\n##################ACA NO HABIA NADA################\n");
	// 	if (free_empty_blocks_success == 0) {
	// 		printfmt("ERROR: Failed freeing a totally empty block.\n");
	// 		return;
	// 	};
	// }
	// else if (curr->size <= MEDIUM_BLOCK_SIZE) {
	// 	if (!free_empty_blocks(&small_size_block_list, MEDIUM_BLOCK_SIZE)) {
	// 		printfmt("ERROR: Failed freeing a totally empty block.\n");
	// 		return;
	// 	}
	// } 
	// else {
	// 	if (!free_empty_blocks(&small_size_block_list, LARGE_BLOCK_SIZE)) {
	// 		printfmt("ERROR: Failed freeing a totally empty block.\n");
	// 		return;
	// 	}
	// }

	print_all_free_list_elements(small_size_block_list);
	// print_all_free_list_elements(medium_size_block_list);
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