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

#define MIN_BLOCK_SIZE 16384  //= 16 Kib

struct region *region_free_list = NULL;

//region1 -> region2 -> region3 

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
			if (prev != NULL) {
				next->free =
				        false;  //-> if the implementation uses the free field
				// prev->next = next->next; // A->B->C => first_fit = B => A->C (A->next = B->next)
			} else {
				next->free =
				        false;  //-> if the implementation uses the free field
				// region_free_list = next->next; //Actualize first node of the free list
			}
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
split_free_regions(struct region *region_to_split, size_t size)
{
	// if region_to_split->size > size: crear region de tam size
	// actualizar la free list (region to split->size = new_size)
	// retornar region del tam q quiero
}


/// Public API of malloc library ///

void *
malloc(size_t size)
{
	struct region *next;

	// aligns to multiple of 4 bytes
	size = ALIGN4(size);

	if (amount_of_mallocs == 0) {
		// printfmt("ENTRO\n");
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

	// Your code here
	//
	// hint: maybe split free regions?

	// next = spilt_free_regions();

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

	// Your code here
	//
	// hint: maybe coalesce regions?
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