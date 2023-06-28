#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>

int
main(int argc, char *argv[])
{
	if (argc != 3) {
		fprintf(stderr, "Usage: %s <filename> <content>\n", argv[0]);
		return 1;
	}

	const char *filename = argv[1];
	const char *content = argv[2];

	// Open the file with O_CREAT flag to create it if it doesn't exist
	// O_WRONLY is for write mode
	// Use 0644 as the file mode (rw-r--r--)
	int fd = open(filename,
	              O_CREAT | O_WRONLY,
	              S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

	printf("foo\n");
	if (fd == -1) {
		perror("open");
		return 1;
	}

	// Write the content to the file, emulating output redirection
	if (write(fd, content, strlen(content)) == -1) {
		perror("write");
		close(fd);
		return 1;
	}

	// Close the file descriptor
	close(fd);

	printf("File %s has been created with content: %s\n", filename, content);

	return 0;
}
