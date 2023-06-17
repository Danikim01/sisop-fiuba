#include <assert.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>

int
main()
{
	int res;
	printf("mkdir_test: - start\n");
	mode_t mode = S_IRUSR | S_IWUSR | S_IRWXO;
	const char *path_directorio = "gran_directorio";
	res = mkdir(path_directorio, mode);
	assert(res == 0);
	printf("mkdir_test: - end\n");

	return 0;
}