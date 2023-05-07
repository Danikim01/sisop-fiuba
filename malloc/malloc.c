#define _DEFAULT_SOURCE

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>

#include "malloc.h"
#include "printfmt.h"
#include <sys/mman.h>

#define MIN_SIZE 256
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

static struct region *
first_fit(size_t size)
{
	if (!first_block) {
		return NULL;
	}
	struct region *current = first_block->first_region;
	struct region *aux = first_block->first_region;
	while(aux){
		printfmt("La region tiene tamaño %d\n",aux->size);
		aux = aux->next;
	}

	while (current) {
		if (current->free && current->size >= size) {
			// Me fijo si puedo hacer el splitting
			if (current->size >=
			    size + MIN_SIZE + sizeof(struct region)) {
				printfmt("Hago el splitting\n");
				struct region *new =
				        (struct region *) ((char *) current + size +
				                           sizeof(struct region));
				new->free = true;
				new->size =
				        current->size - size -
				        sizeof(struct region);  // resto el header de la region
				current->free = false;
				current->size = size;
				current->next = new;

			} else {
				current->free = false;
			}
			return current;
		}
		current = current->next;
	}
	return NULL;
}

// crea una nueva region que ocupa todo el bloque
struct region *
initialize_region(struct region *nueva_region)
{
	nueva_region = (struct region *) first_block->first_region;
	nueva_region->free = true;
	nueva_region->size =
	        first_block->size -
	        sizeof(*first_block) - sizeof(*(first_block->first_region));  // resto el header del bloque con el header de la region
	//printfmt("El tamaño de la region es %d\n", nueva_region->size);
	//printfmt("El tamaño del bloque es %d\n", first_block->size);
	return nueva_region;
}

void *
initialize_block()
{
	void *memoria = mmap(NULL,
	                     BLOCK_SIZE,
	                     PROT_READ | PROT_WRITE,
	                     MAP_PRIVATE | MAP_ANONYMOUS,
	                     -1,
	                     0);
	if (memoria == MAP_FAILED)
		return NULL;
	struct block *nuevo_bloque = (struct block *) memoria;

	// nuevo_bloque->first_region = (struct region *) memoria +
	// sizeof(*nuevo_bloque); establece la posición inicial de la primera
	// región dentro del bloque de memoria recién inicializado.

	nuevo_bloque->first_region =
	        (struct region *) memoria + sizeof(*nuevo_bloque);
	nuevo_bloque->size = BLOCK_SIZE;

	first_block = nuevo_bloque;
}

void *
malloc(size_t size)
{
	struct region *next;
	size = ALIGN4(size);
	amount_of_mallocs++;
	requested_memory += size;

	// Si no hay un bloque, lo tengo que inicializar
	if (!first_block) {
		initialize_block();
	}

	next = first_fit(size);
	// Si no hay una region disponible inicializo una region que ocupa todo el bloque
	if (!next) {
		next = initialize_region(next);
		next = first_fit(size);
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

	// Buscar la región anterior y la siguiente
    struct region *anterior = NULL;
    struct region *siguiente = curr->next;
    struct region *actual = first_block->first_region;

	
	//hago que anterior apunte a la region anterior a la que se va a liberar
	while (actual && actual < curr) {
        anterior = actual;
        actual = actual->next;
    }

	//Si la region anterior a curr esta libre hago coalescing
	if (anterior && (anterior->free==true)) {
		printfmt("La region anterior a curr esta libre");
		printfmt("Junto la region con size %d y size %d\n",anterior->size,curr->size);
        // Coalescing con la región anterior a curr
        anterior->size += curr->size + sizeof(struct region);
        anterior->next = siguiente;
        curr = anterior; //hago que curr apunte a la region juntada.
    }


	if (siguiente && siguiente->free) {
		printfmt("La region siguiente a curr esta libre");
		printfmt("Junto la region con size %d y size %d\n",siguiente->size,curr->size);
        // Coalescing con la región siguiente a curr
        curr->size += siguiente->size + sizeof(struct region);
        curr->next = siguiente->next;
    }

	// Chequear si la región actual es la única región en el bloque
	if (curr == first_block->first_region && curr->next == NULL) {
		// Llamar a munmap() en el bloque completo
		munmap((void*) first_block, first_block->size);
		first_block = NULL;
	}

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
