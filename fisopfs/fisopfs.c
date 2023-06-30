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
#include <time.h>
#include <stdbool.h>

#define INODE_AMOUNT 100
#define BLOCK_DATA_AMOUNT 400
#define DATABLOCKMAX 4
#define BLOCKSIZE 4096
#define MAX_NAME 60

struct block {
	bool is_free;
	char content[BLOCKSIZE];
};

struct inode {
	off_t size;
	mode_t mode;
	time_t date_of_creation;
	time_t date_of_modification;
	time_t data_of_access;
	char name[MAX_NAME];
	uid_t st_uid;
	struct block data[DATABLOCKMAX];
	bool is_file;
	bool is_free;
};

struct block data_blocks[BLOCK_DATA_AMOUNT];
struct inode inodes[INODE_AMOUNT];

char *archivo = "myfs.fisopfs";

int search_inode_with_path(const char *path);
int create_file(const char *path, mode_t mode, bool tipo);
int count_slash(const char *name);
int search_slash(const char *name);
int comp_str(const char *string1, char *string2);
void update_time(const char *path);
int count_blocks_allocated(struct inode inodo);
static void remove_directory_contents(const char *path);

int
minimo(int a, int b)
{
	if (a < b)
		return a;
	return b;
}


static void
init_file()
{
	int valor = 0;
	FILE *file;

	file = fopen(archivo, "w");
	if (file == NULL) {
		perror("Failed to open file\n");
		return;
	}


	for (int i = 0; i < BLOCK_DATA_AMOUNT; i++) {
		memset(&data_blocks[i], 0, sizeof(struct block));
		data_blocks[i].is_free = true;
	}

	for (int i = 0; i < INODE_AMOUNT; i++) {
		memset(&inodes[i], 0, sizeof(struct inode));
		inodes[i].is_free = true;
	}

	for (int i = 0; i < BLOCK_DATA_AMOUNT; i++) {
		valor = fwrite(&data_blocks[i], sizeof(struct block), 1, file);
		if (valor < 0) {
			perror("Write error\n");
			fclose(file);
			return;
		}
	}

	for (int i = 0; i < INODE_AMOUNT; i++) {
		valor = fwrite(&inodes[i], sizeof(struct inode), 1, file);
		if (valor < 0) {
			perror("Write error\n");
			fclose(file);
			return;
		}
	}

	fclose(file);
}

static void
load_file_from_disk(FILE *file)
{
	int valor = 0;

	for (int i = 0; i < BLOCK_DATA_AMOUNT; i++) {
		valor = fread(&data_blocks[i], sizeof(data_blocks[i]), 1, file);
		if (valor < 0) {
			perror("Error al leer el data blocks\n");
		}
	}

	for (int i = 0; i < INODE_AMOUNT; i++) {
		memset(&inodes[i].data, 0, sizeof(struct inode));
		valor = fread(&inodes[i], sizeof(inodes[i]), 1, file);
		if (valor < 0) {
			perror("Error al leer el inodes\n");
		}
	}
}


static void *
fisop_init(struct fuse_conn_info *conn, struct fuse_config *cfg)
{
	printf("[debug] fisop_init\n");
	FILE *file;
	file = fopen(archivo, "r");
	if (!file) {
		init_file();
	} else {
		load_file_from_disk(file);
		fclose(file);
	}
	void *return_value = 0;
	return return_value;
}


void
update_file()
{
	FILE *file;
	file = fopen(archivo, "w");

	for (int i = 0; i < BLOCK_DATA_AMOUNT; i++) {
		if (fwrite(&data_blocks[i], sizeof(struct block), 1, file) < 0) {
			perror("Write error\n");
			fclose(file);
			return;
		}
	}

	for (int i = 0; i < INODE_AMOUNT; i++) {
		if (fwrite(&inodes[i], sizeof(struct inode), 1, file) < 0) {
			perror("Write error\n");
			fclose(file);
			return;
		}
	}

	fclose(file);
}

static int
fisopfs_getattr(const char *path, struct stat *st)
{
	printf("[debug] fisopfs_getattr(%s)\n", path);

	if (strcmp(path, "/") == 0) {
		st->st_uid = getuid();
		st->st_gid = getgid();
		st->st_mode = __S_IFDIR | 0755;
		st->st_nlink = 2;
	} else {
		int indice_inodo = search_inode_with_path(path);
		if (indice_inodo < 0) {
			return -ENOENT;
		}
		if (inodes[indice_inodo].is_file) {
			st->st_size = inodes[indice_inodo].size;
			st->st_mode = __S_IFREG | 0644;
			st->st_nlink = 1;
			st->st_blksize = BLOCKSIZE;
			st->st_blocks =
			        count_blocks_allocated(inodes[indice_inodo]);
		} else {
			st->st_mode = __S_IFDIR | 0755;
			st->st_nlink = 2;
		}
		st->st_uid = inodes[indice_inodo].st_uid;
		st->st_gid = getgid();
		st->st_atime = inodes[indice_inodo].data_of_access;
		st->st_mtime = inodes[indice_inodo].date_of_modification;
		st->st_ctime = inodes[indice_inodo].date_of_modification;
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
	printf("[debug] fisopfs_readdir(%s)\n", path);

	filler(buffer, ".", NULL, 0);
	filler(buffer, "..", NULL, 0);

	if (strcmp(path, "/") == 0) {
		for (int i = 0; i < INODE_AMOUNT; i++) {
			if (!inodes[i].is_free &&
			    count_slash(inodes[i].name) == 1) {
				filler(buffer, inodes[i].name + 1, NULL, 0);
			}
		}
	} else {
		int slashes = count_slash(path);
		for (int i = 0; i < INODE_AMOUNT; i++) {
			if (!inodes[i].is_free && comp_str(path, inodes[i].name)) {
				if (count_slash(inodes[i].name) == slashes + 1) {
					filler(buffer,
					       inodes[i].name +
					               search_slash(inodes[i].name),
					       NULL,
					       0);
				}
			}
		}
	}

	return 0;
}


static int
read_from_block(struct block *block, char *buffer, size_t size, off_t offset_within_block)
{
	size_t bytes_to_read = minimo(size, BLOCKSIZE - offset_within_block);
	memcpy(buffer, block->content + offset_within_block, bytes_to_read);
	return bytes_to_read;
}

static int
fisop_read(const char *path,
           char *buffer,
           size_t size,
           off_t offset,
           struct fuse_file_info *fi)
{
	printf("[debug] fisop_read(%s, %lu, %lu)\n", path, offset, size);

	int indice = search_inode_with_path(path);
	if (indice < 0) {
		return -ENOENT;
	}

	inodes[indice].data_of_access = time(NULL);

	if (offset >= inodes[indice].size) {
		return 0;
	}

	size_t remaining_size = inodes[indice].size - offset;
	size_t read_size = minimo(size, remaining_size);
	if (read_size == 0) {
		return 0;  // File is empty, return 0 bytes
	}

	off_t block_offset = offset / BLOCKSIZE;
	off_t block_offset_within_block = offset % BLOCKSIZE;
	size_t bytes_read = 0;

	while (read_size > 0 && block_offset < DATABLOCKMAX &&
	       !inodes[indice].is_free) {
		struct block *block = &inodes[indice].data[block_offset];
		bytes_read += read_from_block(block,
		                              buffer + bytes_read,
		                              read_size,
		                              block_offset_within_block);
		read_size -= bytes_read;
		block_offset++;
		block_offset_within_block = 0;
	}

	return bytes_read;
}


static int
fisopfs_createfiles(const char *path, mode_t mode, struct fuse_file_info *fi)
{
	printf("[debug] fisopfs_createfiles(%s)\n", path);
	return create_file(path, mode, true);
}

static int
fisop_createdir(const char *path, mode_t mode)
{
	printf("[debug] fisop_createdir(%s) con modo: %d\n", path, mode);
	return create_file(path, mode, false);
}

int
search_free_block(int indice, int offset)
{
	int i = 0;
	while (i < BLOCK_DATA_AMOUNT) {
		if (data_blocks[i].is_free) {
			inodes[indice].data[offset] = data_blocks[i];
			return 0;
		}
		i++;
	}
	return -1;
}

static int
find_free_block(int inode_index, off_t block_offset)
{
	if (search_free_block(inode_index, block_offset / BLOCKSIZE) == -1) {
		return -ENOSPC;
	}
	return 0;
}


static int
write_to_block(struct inode *inode, const char *message, size_t size, off_t offset)
{
	struct block *block = &inode->data[offset / BLOCKSIZE];
	memcpy(block->content + (offset % BLOCKSIZE),
	       message,
	       minimo(size, BLOCKSIZE - offset % BLOCKSIZE));
	return minimo(size, BLOCKSIZE - offset % BLOCKSIZE);
}

static int
write_remaining_data(int inode_index, const char *message, size_t size, off_t offset)
{
	if (offset + size > strlen(message)) {
		size = strlen(message) - offset;
	}
	size = size > 0 ? size : 0;
	strncpy(inodes[inode_index].data[offset / BLOCKSIZE].content, message, size);
	return 0;
}

static int
fisop_write(const char *path,
            const char *message,
            size_t size,
            off_t offset,
            struct fuse_file_info *fi)
{
	printf("[debug] fisop_write(%s)\n", path);
	int inode_index = search_inode_with_path(path);
	if (inode_index < 0) {
		int valor = create_file(path, 0755, true);
		if (valor != 0) {
			return valor;
		}
		inode_index = search_inode_with_path(path);
	}

	if (!inodes[inode_index].is_file) {
		errno = EISDIR;
		return -EISDIR;
	}

	if ((inodes[inode_index].mode & S_IWUSR) == 0) {
		errno = EACCES;
		return -EACCES;
	}

	inodes[inode_index].size =
	        minimo(offset + size, BLOCKSIZE * DATABLOCKMAX);
	inodes[inode_index].date_of_modification = time(NULL);
	inodes[inode_index].data_of_access = time(NULL);
	update_time(path);

	while (offset < (BLOCKSIZE * DATABLOCKMAX)) {
		if (!inodes[inode_index].is_free) {
			int bytes_written = write_to_block(
			        &inodes[inode_index], message, size, offset);
			return bytes_written;
		} else {
			if (find_free_block(inode_index, offset) == -1) {
				return -ENOSPC;
			}
		}
	}
	return write_remaining_data(inode_index, message, size, offset);
}


static int
fisop_removefile(const char *path)
{
	printf("[debug] fisop_removefile(%s)\n", path);

	int indice = search_inode_with_path(path);
	if (indice < 0) {
		errno = ENOENT;
		return -ENOENT;
	}

	if (inodes[indice].is_file == false) {
		errno = EISDIR;
		return -EISDIR;
	}

	for (int j = 0; j < DATABLOCKMAX; j++) {
		if (inodes[indice].is_free == false) {
			memset(data_blocks[j].content, 0, BLOCKSIZE);
			memset(&data_blocks[j], 0, sizeof(struct block));
			data_blocks[j].is_free = true;
		}
	}

	memset(&inodes[indice], 0, sizeof(struct inode));
	inodes[indice].is_free = true;

	update_time(path);

	return 0;
}


static void
remove_directory_blocks(int index)
{
	for (int j = 0; j < DATABLOCKMAX; j++) {
		if (inodes[index].is_free == false) {
			memset(data_blocks[j].content, 0, BLOCKSIZE);
			memset(&data_blocks[j], 0, sizeof(struct block));
			data_blocks[j].is_free = true;
		}
	}
}


int
search_inode_with_path(const char *path)
{
	for (int i = 0; i < INODE_AMOUNT; i++) {
		if (strcmp(inodes[i].name, path) == 0) {
			return i;
		}
	}
	return -1;
}

static int
fisop_removedir(const char *path)
{
	printf("[debug] fisop_removedir(%s)\n", path);

	int index = search_inode_with_path(path);
	if (index < 0) {
		errno = ENOENT;
		return -ENOENT;
	}

	if (inodes[index].is_file) {
		errno = ENOTDIR;
		return -ENOTDIR;
	}

	remove_directory_contents(path);
	remove_directory_blocks(index);
	memset(&inodes[index], 0, sizeof(struct inode));
	inodes[index].is_free = true;
	update_time(path);
	return 0;
}

static void
remove_file_or_directory(const char *path)
{
	int index = search_inode_with_path(path);
	if (inodes[index].is_file) {
		fisop_removefile(path);
	} else {
		fisop_removedir(path);
	}
}

static void
remove_directory_contents(const char *path)
{
	for (int i = 0; i < INODE_AMOUNT; i++) {
		if (!inodes[i].is_free && comp_str(path, inodes[i].name)) {
			if (inodes[i].is_file &&
			    count_slash(inodes[i].name) > count_slash(path)) {
				remove_file_or_directory(inodes[i].name);
			} else if (!inodes[i].is_file &&
			           strcmp(inodes[i].name, path) > 0) {
				remove_directory_contents(inodes[i].name);
			}
		}
	}
}


static int
fisop_utimens(const char *path_archivo,
              const struct timespec tv[2],
              struct fuse_file_info *fi)
{
	printf("[debug] fisop_utimens\n");
	int indice_inodo = search_inode_with_path(path_archivo);
	inodes[indice_inodo].data_of_access = tv[0].tv_sec;
	inodes[indice_inodo].date_of_modification = tv[1].tv_sec;
	return 0;
}

static void
fisop_destroy(void *data)
{
	printf("[debug] fisop_destroy\n");
	update_file();
}

static int
fisop_flush(const char *path, struct fuse_file_info *fi)
{
	printf("[debug] fisop_flush\n");
	update_file();
	return 0;
}

static struct fuse_operations operations = {
	.create = fisopfs_createfiles,
	.getattr = fisopfs_getattr,
	.init = fisop_init,
	.mkdir = fisop_createdir,
	.read = fisop_read,
	.readdir = fisopfs_readdir,
	.write = fisop_write,
	.unlink = fisop_removefile,
	.rmdir = fisop_removedir,
	.utimens = fisop_utimens,
	.destroy = fisop_destroy,
	.flush = fisop_flush,

};

int
main(int argc, char *argv[])
{
	// fuse_main es la funcion que se encarga de montar el filesystem
	return fuse_main(argc, argv, &operations, NULL);
}

struct inode
create_inode(const char *path, mode_t mode, bool tipo, int inode_index, int block_index)
{
	struct inode archivo;
	archivo.mode = mode;
	archivo.date_of_creation = time(NULL);
	archivo.data_of_access = time(NULL);
	archivo.date_of_modification = time(NULL);
	archivo.data[0] = data_blocks[block_index];
	archivo.st_uid = getuid();
	archivo.size = 0;
	archivo.is_file = tipo;
	archivo.is_free = false;
	strcpy(archivo.name, path);
	return archivo;
}

int
create_file(const char *path, mode_t mode, bool tipo)
{
	int inode_index = 0;
	int block_index = 0;

	while (block_index < BLOCK_DATA_AMOUNT &&
	       !data_blocks[block_index].is_free) {
		block_index++;
	}
	while (inode_index < INODE_AMOUNT && !inodes[inode_index].is_free) {
		inode_index++;
	}

	if (inode_index >= INODE_AMOUNT || block_index >= BLOCK_DATA_AMOUNT)
		return -ENOSPC;

	data_blocks[block_index].is_free = false;

	inodes[inode_index] =
	        create_inode(path, mode, tipo, inode_index, block_index);

	update_time(path);

	return 0;
}


int
count_slash(const char *name)
{
	int contador = 0;
	for (int i = 0; name[i] != '\0'; i++) {
		if (name[i] == '/')
			contador++;
	}
	return contador;
}


int
search_slash(const char *name)
{
	int barrita = 0;
	for (int i = 0; name[i] != '\0'; i++) {
		if (name[i] == '/')
			barrita = i;
	}
	return (barrita + 1);
}


int
comp_str(const char *string1, char *string2)
{
	int i = 0;
	while (string1[i] != '\0' && string2[i] != '\0') {
		if (string1[i] != string2[i])
			return 0;
		i++;
	}
	return 1;
}


int
count_blocks_allocated(struct inode inodo)
{
	int count = 0;
	for (int i = 0; i < DATABLOCKMAX; i++) {
		if (!inodo.data[i].is_free) {
			count++;
		}
	}
	return count;
}

void
update_time(const char *path)
{
	int num_slashes = count_slash(path);
	if (num_slashes <= 1) {
		return;
	}

	int index_last_slash = search_slash(path) - 1;
	char parent_dir_name[MAX_NAME];
	strncpy(parent_dir_name, path, index_last_slash);
	parent_dir_name[index_last_slash] = '\0';

	int parent_dir_index = search_inode_with_path(parent_dir_name);
	if (parent_dir_index < 0) {
		return;
	}
	inodes[parent_dir_index].date_of_modification = time(NULL);
}