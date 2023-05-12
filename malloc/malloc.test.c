#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "testlib.h"
#include "malloc.h"


static void
print_test_name(char *test_name)
{
	printfmt(COLOR_GREEN
	         "\n══════════════════════════════════════════\n" COLOR_RESET);
	printfmt(COLOR_GREEN "TEST: %s" COLOR_RESET, test_name);
	printfmt(COLOR_GREEN
	         "\n══════════════════════════════════════════ \n" COLOR_RESET);
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


	return 0;
}