#define _DEFAULT_SOURCE

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>

#include "malloc.h"
#include "printfmt.h"
#include <sys/mman.h>

#define ALIGN4(s) (((((s) -1) >> 2) << 2) + 4)
#define REGION2PTR(r) ((r) + 1)
#define PTR2REGION(ptr) ((struct region *) (ptr) -1)

#define BLOCK_SIZE (16 * 1024)

struct region {
	bool free;
	size_t size;
	struct region *next;
};

struct block {
	size_t size;
	struct region *first_region;
};

struct block *first_block = NULL;
struct region *region_free_list = NULL;

int amount_of_mallocs = 0;
int amount_of_frees = 0;
int requested_memory = 0;

// finds the next free region
// that holds the requested size
//
static struct region *
find_free_region(size_t size)
{
	struct region *next = region_free_list;

#ifdef FIRST_FIT
	// Your code here for "first fit"
	while (next != NULL) {
		if (next->size >= size && free) {
			return next;
		}
		next = next->next;
	}

	return NULL;
#endif

#ifdef BEST_FIT
	// Your code here for "best fit"
#endif

	return next;
}

// static struct region *
// grow_heap(size_t size)
// {
// 	// finds the current heap break
// 	struct region *curr = (struct region *) sbrk(0);

// 	// allocates the requested size
// 	struct region *prev =
// 	        (struct region *) sbrk(sizeof(struct region) + size);

// 	// verifies that the returned address
// 	// is the same that the previous break
// 	// (ref: sbrk(2))
// 	assert(curr == prev);

// 	// verifies that the allocation
// 	// is successful
// 	//
// 	// (ref: sbrk(2))
// 	if (curr == (struct region *) -1) {
// 		return NULL;
// 	}

// 	// first time here
// 	if (!region_free_list) {
// 		region_free_list = curr;
// 	}

// 	curr->size = size;
// 	curr->next = NULL;
// 	curr->free = false;

// 	return curr;
// }

static struct region *
grow_heap(size_t size)
{
	// Se establecen los permisos de lectura/escritura (PROT_READ |
	// PROT_WRITE) y se utilizan las opciones MAP_PRIVATE y MAP_ANONYMOUS
	// para obtener una copia privada y sin archivo de la región de memoria.
	struct region *curr = (struct region *) mmap(NULL,
	                                             sizeof(struct region) + size,
	                                             PROT_READ | PROT_WRITE,
	                                             MAP_PRIVATE | MAP_ANONYMOUS,
	                                             -1,
	                                             0);


	// verifies that the allocation
	// is successful
	//
	if (curr == (struct region *) -1) {
		return NULL;
	}

	curr->size = size;
	curr->next = NULL;
	curr->free = false;

	return curr;
}


/// Public API of malloc library ///

// void *
// malloc(size_t size)
// {
// 	struct region *next;

// 	// aligns to multiple of 4 bytes
// 	size = ALIGN4(size);

// 	// updates statistics
// 	amount_of_mallocs++;
// 	requested_memory += size;

// 	next = find_free_region(size);

// 	if (!next) {
// 		next = grow_heap(size);
// 	}

// 	// Your code here
// 	//
// 	// hint: maybe split free regions?

// 	return REGION2PTR(next);
// }

static struct region *
first_fit(size_t size, struct region *current)
{
	while (current) {
		if (current->free && current->size >= size) {
			current->free = false;
			return current;
		}
		current = current->next;
	}
	// No se encontró ninguna región libre, se necesita asignar una nueva
	// struct region *new_region = grow_heap(size);
	// if (!new_region) {
	// 	return NULL;
	// }
	// // Agregar la nueva región al final del bloque
	// current = new_region;
	// return new_region;
	return NULL;
}

// crea una nueva region que ocupa todo el bloque
struct region *
initialize_region(struct region *nueva_region, struct block *nuevo_bloque)
{
	nueva_region = (struct region *) nuevo_bloque->first_region;
	nueva_region->free = false;
	nueva_region->size =
	        nuevo_bloque->size - sizeof(*nuevo_bloque) -
	        sizeof(*(nuevo_bloque->first_region));
	nueva_region->next = NULL;
	nuevo_bloque->first_region = nueva_region;
	return nueva_region;
}

struct bloque *initialize_block(struct block*nuevo_bloque) {
	void *memoria = 
				mmap(NULL,  
	             BLOCK_SIZE,
	             PROT_READ | PROT_WRITE,
	             MAP_PRIVATE | MAP_ANONYMOUS,
	             -1,  
	             0);
	if (memoria == MAP_FAILED)
		return NULL;
	nuevo_bloque = (struct block*) memoria;  
	
	//nuevo_bloque->first_region = (struct region *) memoria + sizeof(*nuevo_bloque); 
	//establece la posición inicial de la primera región dentro del bloque de memoria recién inicializado.

	nuevo_bloque->first_region = (struct region *) memoria + sizeof(*nuevo_bloque);  
	nuevo_bloque->size = BLOCK_SIZE;

	if (!first_block)
			first_block = nuevo_bloque;
	
	return nuevo_bloque;
}

void *
malloc(size_t size)
{
	struct region *next;
	size = ALIGN4(size);
	amount_of_mallocs++;
	requested_memory += size;
	
	//inicializo el bloque
	struct block *nuevo_bloque = first_block;
	if(nuevo_bloque==NULL){
		nuevo_bloque = initialize_block(NULL);
	}
	
	//Encuentro la primer region que este libre dentro del bloque
	next = first_fit(size, nuevo_bloque->first_region);
	if(next){
		return REGION2PTR(next);
	}

	//En caso contrario, inicializo una region.
	next = initialize_region(next, nuevo_bloque);
	if (!next) {
		return NULL;
	}
	return REGION2PTR(next);
}

void
free(void *ptr)
{
	// updates statistics
	amount_of_frees++;

	struct region *curr = PTR2REGION(ptr);
	assert(curr->free == 0);

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
