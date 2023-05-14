#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "testlib.h"
#include "malloc.h"

// This is just for the print_test_name function
#include <string.h>
#include <unistd.h>
#define STDOUT_FILENO 1
//**********************************************

// add by dani & jm
#define PTR2REGION(ptr) ((struct region *) (ptr) -1)

static void
print_test_name(char *test_name)
{
	int length = strlen(test_name);
	int total_length =
	        length +
	        8;  // Adjust this value according to your desired formatting

	char line[total_length + 1];
	memset(line, '=', total_length);
	line[total_length] = '\0';

	write(STDOUT_FILENO, COLOR_GREEN, strlen(COLOR_GREEN));
	write(STDOUT_FILENO, line, total_length);
	write(STDOUT_FILENO, "\n", 1);
	write(STDOUT_FILENO, "TEST: ", 6);
	write(STDOUT_FILENO, test_name, strlen(test_name));
	write(STDOUT_FILENO, "\n", 1);
	write(STDOUT_FILENO, line, total_length);
	write(STDOUT_FILENO, COLOR_RESET, strlen(COLOR_RESET));
	write(STDOUT_FILENO, "\n", 1);
}

static void
successful_malloc_returns_non_null_pointer(void)
{
	print_test_name("successful_malloc_returns_non_null_pointer");

	char *var = malloc(100);

	ASSERT_TRUE("successful malloc returns non null pointer", var != NULL);

	free(var);
}

static void
correct_copied_value(void)
{
	print_test_name("correct_copied_value");
	char *test_string = "FISOP malloc is working!";

	char *var = malloc(100);

	strcpy(var, test_string);

	ASSERT_TRUE("allocated memory should contain the copied value",
	            strcmp(var, test_string) == 0);

	free(var);
}

static void
correct_amount_of_mallocs(void)
{
	print_test_name("correct_amount_of_mallocs");
	struct malloc_stats stats;

	char *var = malloc(100);
	get_stats(&stats);

	ASSERT_TRUE("amount of mallocs should be one", stats.mallocs == 1);

	printfmt("before malloc 2");
	char *var_2 = malloc(100);
	printfmt("after malloc 2");
	get_stats(&stats);

	ASSERT_TRUE("amount of mallocs should be two", stats.mallocs == 2);
	free(var);
	free(var_2);
}

static void
test_regions_are_updated_to_not_free(void)
{
	print_test_name("test_regions_are_updated_to_not_free");

	char *var = malloc(100);

	struct region *region_header = PTR2REGION(var);

	ASSERT_TRUE("Region should not be free", region_header->free == false);
	free(var);
}


static void
test_regions_are_updated_to_free_after_freeing_them(void)
{
	print_test_name("test_regions_are_updated_to_free_after_freeing_them");

	char *var = malloc(100);
	char *var1 = malloc(100);

	struct region *region_header = PTR2REGION(var);

	ASSERT_TRUE("Region should not be free", region_header->free == false);
	free(var);
	ASSERT_TRUE("Region shoul be free", region_header->free == true);

	free(var1);
}

static void
correct_amount_of_frees(void)
{
	print_test_name("correct_amount_of_frees");
	struct malloc_stats stats;

	char *var = malloc(100);

	free(var);

	get_stats(&stats);

	ASSERT_TRUE("amount of frees should be one", stats.frees == 1);

	char *var_2 = malloc(100);

	free(var_2);

	get_stats(&stats);

	ASSERT_TRUE("amount of frees should be two", stats.frees == 2);
}

static void
correct_amount_of_requested_memory(void)
{
	print_test_name("correct_amount_of_requested_memory");
	struct malloc_stats stats;

	char *var = malloc(100);


	free(var);

	get_stats(&stats);

	ASSERT_TRUE("amount of requested memory should be 256 bytes",
	            stats.requested_memory == 256);
}

static void
correct_amount_of_memory_being_use(void)
{
	print_test_name("correct_amount_of_memory_being_use");
	struct malloc_stats stats;

	char *var = malloc(100);
	get_stats(&stats);
	ASSERT_TRUE("amount of memory being use should be 16384 bytes",
	            stats.memory_being_use == 16384);

	free(var);
	get_stats(&stats);
	ASSERT_TRUE("amount of memory being use should be 0 bytes",
	            stats.memory_being_use == 0);
}

static void
multiple_mallocs_are_made_correctly(void)
{
	print_test_name("correct_amount_of_memory_being_use");
	// Till spliting implemented this test passes when it shouldn't (tests nothing really)
	char *test_string = "FISOP malloc is working!";

	char *var = malloc(100);

	strcpy(var, test_string);

	ASSERT_TRUE("allocated memory should contain the copied value",
	            strcmp(var, test_string) == 0);

	char *test_string2 = "FISOP malloc is working!2";

	char *var2 = malloc(200);

	strcpy(var2, test_string2);

	ASSERT_TRUE("allocated memory should contain the copied value",
	            strcmp(var2, test_string2) == 0);

	char *test_string3 = "FISOP malloc is working!3";

	char *var3 = malloc(200);

	strcpy(var3, test_string3);

	ASSERT_TRUE("allocated memory should contain the copied value",
	            strcmp(var3, test_string3) == 0);


	free(var);
	free(var2);
	free(var3);
}

// run only if on first fit mode
static void
test_first_block_is_medium_size_if_user_asks_more_than_small_size(void)
{
	print_test_name("test_first_block_is_medium_size_if_user_asks_more_"
	                "than_small_size");

	char *var = malloc(17000);

	struct malloc_stats stats;

	get_stats(&stats);
	ASSERT_TRUE("Amount of small blocks should be 0: ",
	            stats.amount_of_small_blocks == 0);

	ASSERT_TRUE("Amount of medium blocks should be 1: ",
	            stats.amount_of_medium_blocks == 1);

	ASSERT_TRUE("Amount of large blocks should be 0: ",
	            stats.amount_of_large_blocks == 0);

	free(var);
}

// run only if on first fit mode
static void
test_first_block_is_large_size_if_user_asks_more_than_medium_size(void)
{
	print_test_name("test_first_block_is_large_size_if_user_asks_more_than_"
	                "medium_size");

	char *var = malloc(1050000);

	struct malloc_stats stats;

	get_stats(&stats);
	ASSERT_TRUE("Amount of small blocks should be 0: ",
	            stats.amount_of_small_blocks == 0);

	ASSERT_TRUE("Amount of medium blocks should be 0: ",
	            stats.amount_of_medium_blocks == 0);

	ASSERT_TRUE("Amount of large blocks should be 1: ",
	            stats.amount_of_large_blocks == 1);

	free(var);
}

static void
test_malloc_should_return_null_if_user_asks_more_than_large_size(void)
{
	print_test_name("test_malloc_should_return_null_if_user_asks_more_than_"
	                "large_size");

	char *var = malloc(33600000);

	struct malloc_stats stats;

	get_stats(&stats);
	ASSERT_TRUE("Amount of small blocks should be 0: ",
	            stats.amount_of_small_blocks == 0);

	ASSERT_TRUE("Amount of medium blocks should be 0: ",
	            stats.amount_of_medium_blocks == 0);

	ASSERT_TRUE("Amount of large blocks should be 0: ",
	            stats.amount_of_large_blocks == 0);

	ASSERT_TRUE("Malloc should return NULL: ", var == NULL);
}


static void
test_deletion_of_block(void)
{
	print_test_name("test_deletion_of_block");
	struct malloc_stats stats;

	char *var = malloc(100);
	get_stats(&stats);
	ASSERT_TRUE("Amount of small blocks should be one: ",
	            stats.amount_of_small_blocks == 1);
	free(var);
	get_stats(&stats);
	ASSERT_TRUE("Amount of small blocks should be zero: ",
	            stats.amount_of_small_blocks == 0);
}


static void
test_spliting(void)
{
	print_test_name("test_spliting");

	char *var = malloc(100);

	struct malloc_stats stats;

	get_stats(&stats);
	ASSERT_TRUE("Amount of regions should be 2: ",
	            stats.amount_of_regions == 2);

	free(var);
}

static void
test_coalecing(void)
{
	print_test_name("test_coalecing");

	char *var = malloc(100);

	struct malloc_stats stats;

	free(var);

	get_stats(&stats);
	ASSERT_TRUE("Amount of regions should be 0: ",
	            stats.amount_of_regions == 0);
}

/********************************************************
 ********************FIRST FIT TESTS**********************
 ********************************************************/

#ifdef FIRST_FIT
static void
test_first_fit_returns_first_adequate_region(void)
{
	print_test_name("test_first_fit_returns_first_adequate_region");

	char *var = malloc(100);
	char *var2 = malloc(200);
	char *var3 = malloc(100);
	char *var4 = malloc(50);
	char *var5 = malloc(100);


	char *desired_region = var2;
	free(var2);
	free(var4);

	// Now region 2 and 4 are free

	char *var6 = malloc(50);
	// this should return me the region in var2

	struct malloc_stats stats;
	get_stats(&stats);
	ASSERT_TRUE("I should get the first free region: ",
	            desired_region == var6);

	free(var);
	free(var3);
	free(var5);
	free(var6);
}
#endif


/********************************************************
 ********************BEST FIT TESTS**********************
 ********************************************************/

#ifdef BEST_FIT
// Reg: mantene el estandar del archivo, si todos los nombres de las
// funciones test arrancan con "test" mantenelo, tambien si arrancan
// llamando a print_test_name([nombre del test])
static void
test_best_fit_returns_first_adequate_region(void)
{
	print_test_name("correct_best_fit_single_region");
	// test single block first
	char *var0 = malloc(300);
	char *var1 = malloc(500);
	char *var2 = malloc(300);
	char *var3 = malloc(400);
	char *var4 = malloc(500);

	char *desired_region = var3;
	free(var1);
	free(var3);

	// esto debberia devolver la region 3
	char *var5 = malloc(300);

	struct region *region_header = PTR2REGION(var5);

	ASSERT_TRUE("allocated best fit region size: ", var5 == desired_region);
	ASSERT_TRUE("allocated best fit region is NOT free: ",
	            region_header->free == false);
	free(var0);
	free(var2);
	free(var4);
	free(var5);
}


// Este test no tiene ningun Assert.
static void
correct_best_fit_various_regions(void)
{
	print_test_name("correct_best_fit_various_regions");
	char *var0 = malloc(700);
	char *var1 = malloc(5000);
	char *var2 = malloc(800);
	char *var3 = malloc(1200);
	char *var4 = malloc(8500);
	char *var5 = malloc(800);
	char *var6 = malloc(3000);
	char *var7 = malloc(100);
	char *var8 = malloc(1800);

	free(var0);
	free(var1);
	free(var2);
	free(var3);
	free(var4);
	free(var5);
	free(var6);
	free(var7);
	free(var8);
}
#endif

static void
test_calloc_allocates_desired_size_of_memory(void)
{
	print_test_name("test_calloc_allocates_desired_size_of_memory");
	char *var = calloc(10, 50);  // should allocate 10x10bytes = 500 bytes

	struct malloc_stats stats;
	get_stats(&stats);

	ASSERT_TRUE("Correct amount of allocations", stats.mallocs == 1);
	ASSERT_TRUE("Correct amount of memory requested",
	            stats.requested_memory == 500);
	free(var);
}

static void
test_memory_allocated_by_calloc_is_initializated_in_0(void)
{
	print_test_name(
	        "test_memory_allocated_by_calloc_is_initializated_in_0");
	char *var = calloc(5, sizeof(int));

	for (int i = 0; i < 5; i++) {
		printfmt("Element %d of calloc allocated memory is 0", i);
		ASSERT_TRUE("", var[i] == 0);
	}

	free(var);
}

static void
test_comportamiento_bloques(void)
{
	print_test_name("test_comportamiento_bloques");
	char *a = malloc(15000);
	char *b = malloc(20000);

	struct region *a_region = PTR2REGION(a);
	struct region *b_region = PTR2REGION(b);

	ASSERT_TRUE("bloque a con tamaño correcto", a_region->size == 15000);

	ASSERT_TRUE("bloque b con tamaño correcto", b_region->size == 20000);

	free(a);
	free(b);
}

static void
test_realloc_to_smaller_size(void)
{
	print_test_name("test_realloc_to_smaller_size");
	// Since this is the first malloc we should have 2 regions in the block
	//  one of 600bytes and one of (16k - 600)bytes
	char *ptr = malloc(600);
	char *original_region = ptr;


	// since Realloc creates a new Region now you have 3 regions in total
	// BUT since the spliting function does coalecing and the new split
	// region is free as well as the one of (16k - 600)bytes you should have
	// 2 regions in total after realloc
	ptr = realloc(ptr, 400);

	struct malloc_stats stats;
	get_stats(&stats);
	ASSERT_TRUE("Amount of regions should be 2", stats.amount_of_regions == 2);

	ASSERT_TRUE("Realloc when shrinking should return me the same ptr",
	            original_region == ptr);

	free(ptr);
}

static void
test_realloc_to_bigger_size_next_region_is_free(void)
{
	print_test_name("test_realloc_to_bigger_size_next_region_is_free");
	char *ptr = malloc(300);
	// since Realloc creates a new Region now you have 3 regions in total
	// BUT then you do coalecing so you end up with 2
	ptr = realloc(ptr, 500);
	char *original_region = ptr;

	struct malloc_stats stats;
	get_stats(&stats);
	ASSERT_TRUE("Amount of regions should be 2", stats.amount_of_regions == 2);
	struct region *region_header = PTR2REGION(ptr);
	ASSERT_TRUE("The size of region should be what the user asked",
	            region_header->size == 500);

	ASSERT_TRUE("Realloc when shrinking should return me the same ptr",
	            original_region == ptr);


	free(ptr);
}

static void
test_realloc_to_bigger_size_next_region_is_NOT_free(void)
{
	print_test_name("test_realloc_to_bigger_size_next_region_is_NOT_free");
	char *ptr = malloc(300);
	char *original_region = ptr;
	char *ptr2 = malloc(300);
	// since Realloc next region is occupied this should create a new region
	// and FREE the original one but since the neighbours are occupied (and
	// one in NULL) you should end up with 4 regions, 2 free, 2 !free
	ptr = realloc(ptr, 500);

	struct malloc_stats stats;
	get_stats(&stats);
	ASSERT_TRUE("Amount of regions should be 4", stats.amount_of_regions == 4);

	struct region *region_header = PTR2REGION(ptr);

	if (region_header == NULL)
		printfmt("Es NULL efectivamente\n");

	// Okey no es NULL regionHeader

	ASSERT_TRUE("The size of region should be what the user asked",
	            region_header->size == 500);

	printfmt("Se mata aca\n");
	ASSERT_TRUE("ptr should not be equal to the original region",
	            ptr != original_region);

	struct region *original_region_header = PTR2REGION(original_region);
	ASSERT_TRUE("original_region should be free to use",
	            original_region_header->free == true);
	free(ptr);
	free(ptr2);
}

static void
test_realloc_should_copy_previous_values()
{
	print_test_name("test");
	void *var = malloc(10);
	strcpy(var, "hola");

	realloc(var, 15);

	ASSERT_TRUE("Content should be 'hola'", strcmp((char *) var, "hola") == 0);
}

int
main(void)
{
	// run_test(successful_malloc_returns_non_null_pointer);
	// run_test(correct_copied_value);
	// run_test(correct_amount_of_mallocs);
	// run_test(test_regions_are_updated_to_not_free);
	// run_test(test_regions_are_updated_to_free_after_freeing_them);
	// run_test(correct_amount_of_frees);
	// run_test(correct_amount_of_requested_memory);
	// run_test(multiple_mallocs_are_made_correctly);
	// run_test(test_first_block_is_medium_size_if_user_asks_more_than_small_size);
	// run_test(test_first_block_is_large_size_if_user_asks_more_than_medium_size);
	// run_test(test_malloc_should_return_null_if_user_asks_more_than_large_size);
	// run_test(test_deletion_of_block);
	// run_test(test_spliting);
	// run_test(test_coalecing);

// Test relacionados a First Fit, recorda usar // make - B - e USE_FF = true
// al compilar
#ifdef FIRST_FIT
	// run_test(test_first_fit_returns_first_adequate_region);
#endif

#ifdef BEST_FIT
	// run_test(test_best_fit_returns_first_adequate_region);
// run_test(correct_best_fit_various_regions);
#endif

	// run_test(test_comportamiento_bloques);
	// run_test(test_calloc_allocates_desired_size_of_memory);
	// run_test(test_memory_allocated_by_calloc_is_initializated_in_0);
	run_test(test_realloc_to_smaller_size);
	run_test(test_realloc_to_bigger_size_next_region_is_free);
	run_test(test_realloc_to_bigger_size_next_region_is_NOT_free);
	run_test(test_realloc_should_copy_previous_values);

	return 0;
}