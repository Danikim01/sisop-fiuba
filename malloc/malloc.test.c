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

	char *var_2 = malloc(100);
	get_stats(&stats);

	ASSERT_TRUE("amount of mallocs should be two", stats.mallocs == 2);
	free(var);
	free(var_2);
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
correct_best_fit_single_region(void)
{
	print_test_name("correct_best_fit_single_region");
	// test single block first (small region)
	char *var0 = malloc(300);
	char *var1 = malloc(500);
	char *var2 = malloc(300);
	char *var3 = malloc(400);
	char *var4 = malloc(500);

	free(var1);
	free(var3);

	char *var5 = malloc(300);

	struct region *res = PTR2REGION(var5);
	// Esto seria mejor que este al principio del test como en first fit
	//  asi el

	printfmt("best fit exclusive test\n");
	ASSERT_TRUE("allocated best fit region size",
	            res->next->size == 100 - sizeof(struct region));
	ASSERT_TRUE("allocated best fit is free", res->next->free == true);
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


int
main(void)
{
	run_test(successful_malloc_returns_non_null_pointer);
	run_test(correct_copied_value);
	run_test(correct_amount_of_mallocs);
	run_test(correct_amount_of_frees);
	run_test(correct_amount_of_requested_memory);
	run_test(multiple_mallocs_are_made_correctly);
	run_test(test_first_block_is_medium_size_if_user_asks_more_than_small_size);
	run_test(test_first_block_is_large_size_if_user_asks_more_than_medium_size);
	run_test(test_malloc_should_return_null_if_user_asks_more_than_large_size);
	run_test(test_deletion_of_block);
	run_test(test_spliting);
	run_test(test_coalecing);

// Test relacionados a First Fit, recorda usar // make - B - e USE_FF = true
// al compilar
#ifdef FIRST_FIT
	run_test(test_first_fit_returns_first_adequate_region);
#endif

#ifdef BEST_FIT
	// run_test(correct_best_fit_single_region);
	// run_test(correct_best_fit_various_regions);
#endif

	// run_test(test_comportamiento_bloques);

	return 0;
}