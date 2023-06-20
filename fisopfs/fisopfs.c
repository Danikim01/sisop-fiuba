#define FUSE_USE_VERSION 30

#include <fuse.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>

#define BLOCK_SIZE 4096

#define MAX_CONTENIDO 100
#define MAX_DIRECTORIOS 5
#define MAX_NOMBRE 8
#define MAX_INODOS 80
#define MAX_BLOQUES 64
#define INODE_BLOCKS 10
static char fisop_file_contenidos[MAX_CONTENIDO] = "READING: hola fisopfs!\n";

struct block {
	char contenido[BLOCK_SIZE];
};

struct inode {
	char name[MAX_NOMBRE];
	size_t file_size;
	// Propietario y derechos de acceso
	uid_t owner;
	mode_t permissions;

	// Tiempos de acceso y modificación
	time_t access_time;
	time_t modified_time;

	// Bloques a los que hace referencia este inodo
	struct block *blocks[INODE_BLOCKS];
};

struct superblock {
	// Cantidad de bloques en el disco
	size_t block_count;

	// Cantidad de bloques libres
	size_t free_block_count;

	// Cantidad de inodos en el disco
	size_t inode_count;

	// Cantidad de inodos libres
	size_t free_inode_count;

	// Bitmap de bloques libres
	struct block *free_block_bitmap;

	// Bitmap de inodos libres
	struct block *free_inode_bitmap;

	// Inodos del disco (16 inodos en 5 bloque para inodos)
	struct inode *inodes[MAX_INODOS];

	// Bloques del disco
	struct block *blocks[MAX_BLOQUES];
};

struct superblock superblock;
struct inode *inodes;
struct block *blocks;

void
initialize_file_system()
{
	// inicializar los campos del file system
}

static int
fisopfs_getattr(const char *path, struct stat *st)
{
	printf("[debug] fisopfs_getattr - path: %s\n", path);

	if (strcmp(path, "/") == 0) {
		st->st_uid = 1717;
		st->st_mode = __S_IFDIR | 0755;
		st->st_nlink = 2;
	} else if (strcmp(path, "/fisop") == 0) {
		st->st_uid = 1818;
		st->st_mode = __S_IFREG | 0644;
		st->st_size = 2048;
		st->st_nlink = 1;
	} else {
		printf("NUESTRO: ERROR\n");
		return -ENOENT;
	}

	return 0;
}

static int
fisopfs_readdir(const char *path,
                void *buffer,
                fuse_fill_dir_t filler,
                off_t offset,
                struct fuse_file_info *fi)
{
	printf("[debug] fisopfs_readdir - path: %s\n", path);

	// Los directorios '.' y '..'
	filler(buffer, ".", NULL, 0);
	filler(buffer, "..", NULL, 0);

	// Si nos preguntan por el directorio raiz, solo tenemos un archivo
	if (strcmp(path, "/") == 0) {
		filler(buffer, "fisop", NULL, 0);
		return 0;
	}

	return -ENOENT;
}


static int
fisopfs_mkdir(const char *path, mode_t mode)
{
	printf("[debug] fisopfs_mkdir - path: %s - mode: %d\n", path, mode);
	return 0;
}

static int
fisopfs_read(const char *path,
             char *buffer,
             size_t size,
             off_t offset,
             struct fuse_file_info *fi)
{
	printf("[debug] fisopfs_read - path: %s, offset: %lu, size: %lu\n",
	       path,
	       offset,
	       size);

	// Solo tenemos un archivo hardcodeado!
	if (strcmp(path, "/fisop") != 0)
		return -ENOENT;

	if (offset + size > strlen(fisop_file_contenidos))
		size = strlen(fisop_file_contenidos) - offset;

	size = size > 0 ? size : 0;

	strncpy(buffer, fisop_file_contenidos + offset, size);

	return size;
}

static int
fisopfs_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
	int fd;
	printf("[debug] fisopfs_create - path: %s - mode: %d\n", path, mode);
	// TODO: Add custom logic for the underlying storage.
	//(creating entries in some data structure that represents the files in the filesystem)

	fd = open(path, fi->flags, mode);
	if (fd == -1) {
		return -errno;
	}

	close(fd);
	return 0;
}

static struct fuse_operations operations = {
	.getattr = fisopfs_getattr,
	.readdir = fisopfs_readdir,
	.read = fisopfs_read,
	.mkdir = fisopfs_mkdir,
	.create = fisopfs_create,
};

int
main(int argc, char *argv[])
{
	return fuse_main(argc, argv, &operations, NULL);
}
