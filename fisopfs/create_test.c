#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

int
main(int argc, char *argv[])
{
	const char *path = "testfile.txt";
	const char *message = "Hello, World!";
	size_t size = strlen(message);
	off_t offset = 0;

	// Write data to the file using the write syscall
	ssize_t bytes_written = fisop_write(path, message, size, offset, NULL);
	printf("Wrote %ld bytes\n", bytes_written);
}
