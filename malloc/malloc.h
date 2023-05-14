#ifndef MALLOC_H
#define MALLOC_H

#include <stdbool.h>

struct malloc_stats {
	int mallocs;
	int frees;
	int requested_memory;
	int memory_being_use;
	int amount_of_regions;
	int amount_of_small_blocks;
	int amount_of_medium_blocks;
	int amount_of_large_blocks;
};


struct region {     // Ocupies 32 bytes in memory
	int magic;  // Magic number to check if the region is valid
	bool free;  // This aproach is aparently better than the one in the given text
	size_t size;  // The actual regions size, does not include the header
	struct region *next;
	struct region *prev;  // Exclusively to facilitate coalecing
};


struct block {
	struct region *first_region;
	struct block *next;
	// suggestion add max_size, answer: no need since we have 3 block lists.
};


void *malloc(size_t size);

void free(void *ptr);

void *calloc(size_t nmemb, size_t size);

void *realloc(void *ptr, size_t size);

void get_stats(struct malloc_stats *stats);

void actualize_free_region_list(struct region *new_region);

// Given a region to split, it shrinks it to the desired_size, accomodates the
// free list and returns the newly shrinked region
// static struct region *split_free_regions(struct region *region_to_split,
//                                          size_t desired_size);

#endif  // _MALLOC_H_
