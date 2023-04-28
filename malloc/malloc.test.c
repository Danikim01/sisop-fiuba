#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "testlib.h"
#include "malloc.h"

static void
successful_malloc_returns_non_null_pointer(void)
{
	char *var = malloc(100);

	ASSERT_TRUE("successful malloc returns non null pointer", var != NULL);

	free(var);
}

static void
correct_copied_value(void)
{
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
	struct malloc_stats stats;

	char *var = malloc(100);

	free(var);

	get_stats(&stats);

	ASSERT_TRUE("amount of mallocs should be one", stats.mallocs == 1);
}

static void
correct_amount_of_frees(void)
{
	struct malloc_stats stats;

	char *var = malloc(100);

	free(var);

	get_stats(&stats);

	ASSERT_TRUE("amount of frees should be one", stats.frees == 1);
}

static void
correct_amount_of_requested_memory(void)
{
	struct malloc_stats stats;

	char *var = malloc(100);

	free(var);

	get_stats(&stats);

	ASSERT_TRUE("amount of requested memory should be 100",
	            stats.requested_memory == 100);
}

static void
hago_varios_mallocs(void)
{
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

int
main(void)
{
	run_test(successful_malloc_returns_non_null_pointer);
	run_test(correct_copied_value);
	run_test(correct_amount_of_mallocs);
	run_test(correct_amount_of_frees);
	run_test(correct_amount_of_requested_memory);
	
	
	run_test(hago_varios_mallocs);
	return 0;
}
