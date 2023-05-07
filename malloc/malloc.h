#ifndef MALLOC_H
#define MALLOC_H

#include <stdbool.h>

struct malloc_stats {
	int mallocs;
	int frees;
	int requested_memory;
};


struct region {
	bool free;  // This aproach is aparently better than the one in the given text
	size_t size;  // The actual regions size, does not include the header
	struct region *next;
};


void *malloc(size_t size);

void free(void *ptr);

void *calloc(size_t nmemb, size_t size);

void *realloc(void *ptr, size_t size);

void get_stats(struct malloc_stats *stats);

void actualize_free_region_list(struct region *new_region);


#endif  // _MALLOC_H_
