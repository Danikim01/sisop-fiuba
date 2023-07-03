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
#include <string.h>

#define INODE_AMOUNT 100
#define BLOCK_DATA_AMOUNT 100
#define DATABLOCKMAX 1
#define BLOCKSIZE 4096
#define MAX_NAME 60
#define MAX_NAME_DIR 55
#define MAX_DIRECTORIES 5

struct block {
	bool is_free;
	char content[BLOCKSIZE];
	int inode_index;  // indice del inodo que apunta a este bloque (es -1 si
	                  // no hay ningun inodo que apunte a este bloque)
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
	bool is_symlink;
	int inode_parent_index;
	char target_path[MAX_NAME];
};

struct block data_blocks[BLOCK_DATA_AMOUNT];
struct inode inodes[INODE_AMOUNT];
char *archivo = "myfs.fisopfs";

int search_inode_with_path(const char *path);
struct inode create_inode(const char *path,
                          mode_t mode,
                          bool tipo,
                          int block_index,
                          int parent_index,
                          int index,
                          bool is_sysmlink);
int create_file(const char *path, mode_t mode, bool tipo, bool is_sysmlink);
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
		data_blocks[i].inode_index = -1;
	}

	for (int i = 0; i < INODE_AMOUNT; i++) {
		memset(&inodes[i], 0, sizeof(struct inode));
		inodes[i].is_free = true;
		for (int j = 0; j < DATABLOCKMAX; j++) {
			inodes[i].data[j].is_free = true;
			inodes[i].data[j].inode_index = -1;
		}
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

int
amount_of_files_in_dir(const char *path)
{
	int amount = 0;
	if (count_slash(path) == 1) {
		amount = -1;
	}
	int index_dir = search_inode_with_path(path);

	for (int i = 0; i < INODE_AMOUNT; i++) {
		if (!inodes[i].is_free &&
		    inodes[i].inode_parent_index == index_dir) {
			amount++;
		}
	}
	return amount;
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
		if (inodes[indice_inodo].is_symlink) {
			st->st_mode = __S_IFLNK | 0777;
			st->st_nlink = 1;
			st->st_size = inodes[indice_inodo].size;
		} else if (inodes[indice_inodo].is_file) {
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
	// Delimitamos un limite a la longuitud del path
	if (strlen(path) > MAX_NAME) {
		errno = ENAMETOOLONG;
		return -ENAMETOOLONG;
	}
	return create_file(path, mode, true, false);
}

static int
fisop_createdir(const char *path, mode_t mode)
{
	printf("[debug] fisop_createdir(%s) con modo: %d\n", path, mode);
	if (strlen(path) > (MAX_NAME_DIR)) {
		errno = ENAMETOOLONG;
		return -ENAMETOOLONG;
	}
	int valor = create_file(path, mode, false, false);
	return valor;
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
		int valor = create_file(path, 0755, true, false);
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
		if (inodes[inode_index].data[offset / BLOCKSIZE].inode_index >= 0) {
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

	memset(&inodes[indice], 0, sizeof(struct inode));
	for (int j = 0; j < DATABLOCKMAX; j++) {
		memset(inodes[indice].data[j].content, 0, BLOCKSIZE);
		memset(&inodes[indice].data[j], 0, sizeof(struct block));
		inodes[indice].data[j].is_free = true;
		inodes[indice].data[j].inode_index = -1;
	}

	inodes[indice].is_free = true;

	for (int i = 0; i < BLOCK_DATA_AMOUNT; i++) {
		if (data_blocks[i].inode_index == indice) {
			memset(data_blocks[i].content, 0, BLOCKSIZE);
			memset(&data_blocks[i], 0, sizeof(struct block));
			data_blocks[i].is_free = true;
			data_blocks[i].inode_index = -1;
		}
	}

	update_time(path);

	return 0;
}


int
remove_directory_blocks(int indice)
{
	if (inodes[indice].is_file) {
		errno = ENOTDIR;
		return -ENOTDIR;
	}

	memset(&inodes[indice], 0, sizeof(struct inode));
	for (int j = 0; j < DATABLOCKMAX; j++) {
		memset(inodes[indice].data[j].content, 0, BLOCKSIZE);
		memset(&inodes[indice].data[j], 0, sizeof(struct block));
		inodes[indice].data[j].is_free = true;
		inodes[indice].data[j].inode_index = -1;
	}

	inodes[indice].is_free = true;

	for (int i = 0; i < BLOCK_DATA_AMOUNT; i++) {
		if (data_blocks[i].inode_index == indice) {
			memset(data_blocks[i].content, 0, BLOCKSIZE);
			memset(&data_blocks[i], 0, sizeof(struct block));
			data_blocks[i].is_free = true;
			data_blocks[i].inode_index = -1;
		}
	}

	return 0;
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

	if (amount_of_files_in_dir(path) == 0) {
		remove_directory_contents(path);
		remove_directory_blocks(index);
		// memset(&inodes[index], 0, sizeof(struct inode));
		inodes[index].is_free = true;
		update_time(path);
		return 0;
	}

	errno = ENOTEMPTY;
	return -ENOTEMPTY;
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

static int
fisop_truncate(const char *path_archivo, off_t offset, struct fuse_file_info *fi)
{
	printf("[debug] fisop_truncate\n");
	return 0;
}


/** Read the target of a symbolic link
 *
 * The buffer should be filled with a null terminated string.  The
 * buffer size argument includes the space for the terminating
 * null character.	If the linkname is too long to fit in the
 * buffer, it should be truncated.	The return value should be 0
 * for success.
 */
static int
fisop_readlink(const char *path, char *buf, size_t size)
{
	printf("[debug] fisop_readlink(%s)\n", path);

	int inode_index = search_inode_with_path(path);
	if (inode_index == -1) {
		return -ENOENT;
	}

	if (!inodes[inode_index].is_symlink) {
		return -EINVAL;  // El archivo no es un enlace simbólico
	}

	if (strlen(inodes[inode_index].target_path) >= size) {
		return -ENAMETOOLONG;  // El buffer de destino no es lo suficientemente grande
	}

	strncpy(buf, inodes[inode_index].target_path, size);
	buf[size - 1] = '\0';

	return 0;
}


void
extract_last_component(const char *path, char *buffer, size_t buffer_size)
{
	const char *last_slash = strrchr(path, '/');
	if (last_slash != NULL) {
		// Avanzar después de la última barra diagonal
		last_slash++;

		// Copiar el último componente al buffer
		strncpy(buffer, last_slash, buffer_size - 1);
		buffer[buffer_size - 1] = '\0';
	} else {
		// No se encontró ninguna barra diagonal, copiar todo el path
		strncpy(buffer, path, buffer_size - 1);
		buffer[buffer_size - 1] = '\0';
	}
}


static int
fisop_symlink(const char *from, const char *to)
{
	printf("[debug] fisop_symlink\n");
	printf("from: %s\n", from);
	printf("to: %s\n", to);


	char from_with_slash[MAX_NAME];
	// Para el to se debe eliminar todo lo que venga antes del ulitmo string
	// Ej si to es /carpeta/enlace_simbolico se debe eliminar /carpeta y quedar
	// solo /enlace_simbolico para el nombre del archivo (realizar con memoria estatica)
	char filename[MAX_NAME];
	extract_last_component(to, filename, sizeof(filename));
	char filename_with_slash[MAX_NAME];
	strcpy(filename_with_slash, "/");
	strcat(filename_with_slash, filename);
	strcat(filename_with_slash, "\0");
	printf("filename: %s\n", filename_with_slash);

	if (count_slash(to) <= 1) {
		// Si estoy en raiz concatenar un / al principio del from de tal
		// forma que quede /from usando memoria estatica
		strcpy(from_with_slash, "/");
		strcat(from_with_slash, from);
		strcat(from_with_slash, "\0");
	} else {
		// Tengo que de alguna forma hallar el path original del archivo
		// from Si por ejemplo a.txt esta en carpeta/a.txt entonces a
		// search_inode_with_path le tengo que pasar /carpeta/a.txt
		// Entonces a a.txt le tengo que concatenar al inicio /carpeta/
		// y al final \0 esto lo puedo hacer sabiendo que to contiene el
		// path del archivo ej: si to es /carpeta/enlace_simbolico
		// entonces a a.txt le tengo que concatenar /carpeta/ para que
		// me quede /carpeta/a.txt
		strcpy(from_with_slash, to);
		// en from with lash ahora tengo /carpeta/enlace_simbolico tengo
		// que eliminar enlace_simbolico y en su lugar poner from
		char *last_slash = strrchr(from_with_slash, '/');
		if (last_slash) {
			last_slash[1] =
			        '\0';  // Elimina la parte final (enlace_simbolico) agregada a través de 'to'
			strcat(from_with_slash, from);
		}

		// tengo que hacer exactamente lo mismo para filename_with_slash
		// aca filename  tiene enlace_simbolico le tengo que concatenar
		// /una_carpeta/enlace_simbolico Hacer lo mismo para
		// 'filename_with_slash'
		strcpy(filename_with_slash, to);

		last_slash = strrchr(filename_with_slash, '/');
		if (last_slash) {
			last_slash[1] =
			        '\0';  // Eliminar la parte final (enlace_simbolico) agregada a través de 'to'
			strcat(filename_with_slash, filename);
		}
	}


	printf("from_with_slash: %s\n", from_with_slash);
	printf("filename_with_slash: %s\n", filename_with_slash);

	int from_index = search_inode_with_path(from_with_slash);
	if (from_index < 0) {
		printf("error\n");
		errno = ENOENT;
		return -ENOENT;
	}


	printf("The name of the file from index is %s\n", inodes[from_index].name);


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
	data_blocks[block_index].inode_index = inode_index;

	struct inode archivo = inodes[inode_index];
	archivo.mode = __S_IFLNK | 0777;
	archivo.date_of_creation = time(NULL);
	archivo.data_of_access = time(NULL);
	archivo.date_of_modification = time(NULL);
	archivo.data[0] = data_blocks[block_index];

	archivo.st_uid = getuid();
	archivo.size = strlen(from);
	archivo.is_file = true;
	archivo.is_free = false;
	archivo.inode_parent_index = 0;
	archivo.is_symlink = true;
	strcpy(archivo.name, filename_with_slash);

	strncpy(archivo.target_path, from, sizeof(from_with_slash));

	printf("the name of the symbolic link file is %s\n", archivo.name);
	printf("the target name of the  symbolic link file is %s\n",
	       archivo.target_path);
	printf("the size of the file is %ld\n", archivo.size);

	printf("agrego el archivo en el index %d\n", inode_index);
	inodes[inode_index] = archivo;

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
	.truncate = fisop_truncate,
	.symlink = fisop_symlink,
	.readlink = fisop_readlink,
};

int
main(int argc, char *argv[])
{
	// fuse_main es la funcion que se encarga de montar el filesystem
	return fuse_main(argc, argv, &operations, NULL);
}

struct inode
create_inode(const char *path,
             mode_t mode,
             bool tipo,
             int block_index,
             int index_parent,
             int index,
             bool is_symlink)
{
	struct inode archivo = inodes[index];
	archivo.mode = mode;
	archivo.date_of_creation = time(NULL);
	archivo.data_of_access = time(NULL);
	archivo.date_of_modification = time(NULL);
	archivo.data[0] = data_blocks[block_index];
	archivo.st_uid = getuid();
	archivo.size = 0;
	archivo.is_file = tipo;
	archivo.is_free = false;
	archivo.inode_parent_index = index_parent;
	archivo.is_symlink = is_symlink;
	printf("Creando un archivo con nombre %s\n", path);
	strcpy(archivo.name, path);
	printf("El nombre del archivo es %s\n", archivo.name);
	strcpy(archivo.target_path, " ");
	return archivo;
}

int
create_file(const char *path, mode_t mode, bool tipo, bool is_symlink)
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

	int index_last_slash = search_slash(path) - 1;
	char parent_dir_name[MAX_NAME];
	strncpy(parent_dir_name, path, index_last_slash);
	parent_dir_name[index_last_slash] = '\0';
	int index_parent_inode = search_inode_with_path(parent_dir_name);


	data_blocks[block_index].is_free = false;
	data_blocks[block_index].inode_index = inode_index;

	inodes[inode_index] = create_inode(path,
	                                   mode,
	                                   tipo,
	                                   block_index,
	                                   index_parent_inode,
	                                   inode_index,
	                                   is_symlink);

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