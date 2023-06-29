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

#define CANT_INODOS 100
#define BLOCK_DATA_NUMBER 400
#define DATABLOCKMAX 4  // cantidad de bloques de data que puede tener un inodo
#define BLOCKSIZE 4096  // año de cada bloque
#define MAX_NAME 60     // largo de name de archivo
#define MAX_NOMBRE 60   // largo de nombre de archivo

struct block {
	bool is_free;
	char content[BLOCKSIZE];
	int inode_index;  // indice del inodo que apunta a este bloque (es -1 si
	                  // no hay ningun inodo que apunte a este bloque)
};

struct inode {
	off_t size;
	mode_t mode;  // mode => rwx(owner)r-x(group)r-x(other) con r=read, w=write, x=execute
	time_t date_of_creation;
	time_t date_of_modification;
	time_t data_of_access;
	char name[MAX_NAME];
	uid_t st_uid;
	struct block data[DATABLOCKMAX];  // apunta a los bloques de data (un
	                                  // inodo puede tener mas de un bloque de data)
	bool is_file;  // 0 (false) es directorio, 1 (true) es archivo regular
	bool is_free;
};


struct block data_blocks[BLOCK_DATA_NUMBER];
struct inode inodes[CANT_INODOS];

// name default del archivo que guarda el filesystem
char *archivo = "myfs.fisopfs";

int search_inode_with_path(const char *path);
int crear_archivo(const char *path, mode_t mode, bool tipo);
int count_slash(const char *name);
int search_slash(const char *name);
int comp_str(const char *string1, char *string2);
void update_time(const char *path);

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

// devuelve los atributos de un archivo, dado un path y
// un struct stat (contiene los atributos de un archivo, como uid, mode, etc)
static int
fisopfs_getattr(const char *path, struct stat *st)
{
	printf("[debug] fisopfs_getattr(%s)\n", path);

	if (strcmp(path, "/") == 0) {  // si el path es el root
		st->st_uid = 1717;
		st->st_mode =
		        __S_IFDIR |
		        0755;  // 775 = rwx(owner)r-x(group)r-x(other) con r=read, w=write, x=execute
		st->st_nlink = 2;
	} else {
		// buscar el inode con el path y devolver sus data de manera parecida al ejemplo
		int indice_inodo = search_inode_with_path(path);
		if (indice_inodo < 0) {
			return -ENOENT;
		}
		st->st_uid = inodes[indice_inodo].st_uid;
		st->st_mode = __S_IFDIR | 0755;
		st->st_nlink = 2;
		if (inodes[indice_inodo].is_file) {
			st->st_size = inodes[indice_inodo].size;
			st->st_mode = __S_IFREG | 0644;
			st->st_nlink = 1;
			st->st_blksize = BLOCKSIZE;
		}
		st->st_atime = inodes[indice_inodo].data_of_access;
		st->st_mtime = inodes[indice_inodo].date_of_modification;
		st->st_ctime = inodes[indice_inodo].date_of_modification;
	}
	return 0;
}


// readdir lee el contenido de un DIRECTORIO, dado un path y un buffer donde llenar el contenido (ej de uso. ls)
static int
fisopfs_readdir(const char *path,
                void *buffer,
                fuse_fill_dir_t filler,
                off_t offset,
                struct fuse_file_info *fi)
{
	printf("[debug] fisopfs_readdir(%s)\n", path);
	// llena en el buffer el contenido de . (directorio actual) y .. (directorio padre)
	filler(buffer, ".", NULL, 0);
	filler(buffer, "..", NULL, 0);

	if (strcmp(path, "/") == 0) {
		for (int i = 0; i < CANT_INODOS; i++) {
			if (!inodes[i].is_free && count_slash(inodes[i].name) == 1)
				filler(buffer, inodes[i].name + 1, NULL, 0);
		}
		return 0;
	}
	int barritas = count_slash(path);
	for (int i = 0; i < CANT_INODOS; i++) {
		if (!inodes[i].is_free && comp_str(path, inodes[i].name))
			if (count_slash(inodes[i].name) == barritas + 1)
				filler(buffer,
				       inodes[i].name +
				               search_slash(inodes[i].name),
				       NULL,
				       0);
	}
	return 0;

	// Los directorios '.' y '..'
	filler(buffer,
	       ".",
	       NULL,
	       0);  // llena en el buffer el contenido de . (directorio actual)
	filler(buffer, "..", NULL, 0);

	// busco el inodo del directorio
	int indice = search_inode_with_path(path);
	if (indice < 0) {
		return -ENOENT;
	}
	// struct inodo inodo_dir = inodes[indice];
	// Si encuentro algo pero no es un directorio, lanzo una excepcion
	if (inodes[indice].is_file) {
		return -ENOTDIR;
	}
	filler(buffer, inodes[indice].name, NULL, 0);
	return 0;
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
	printf("indice: %d\n", indice);
	if (indice < 0) {
		return -ENOENT;
	}

	inodes[indice].data_of_access = time(NULL);

	if (offset >= inodes[indice].size) {
		return 0;  // El desplazamiento está más allá del final del archivo
	}

	size_t remaining_size = inodes[indice].size - offset;
	size_t read_size = (size < remaining_size) ? size : remaining_size;
	printf("read_size: %lu\n", read_size);
	if (read_size == 0) {
		return 0;  // File is empty, return 0 bytes transferred
	}

	off_t block_offset = offset / BLOCKSIZE;
	off_t block_offset_within_block = offset % BLOCKSIZE;
	size_t bytes_read = 0;

	while (read_size > 0 && block_offset < DATABLOCKMAX &&
	       !inodes[indice].is_free) {
		printf("foo\n");
		struct block *block = &inodes[indice].data[block_offset];
		printf("foo2\n");
		size_t bytes_to_read =
		        (read_size < (BLOCKSIZE - block_offset_within_block))
		                ? read_size
		                : (BLOCKSIZE - block_offset_within_block);
		printf("bytes_to_read: %lu\n", bytes_to_read);

		printf("foo3\n");

		memcpy(buffer + bytes_read,
		       block->content + block_offset_within_block,
		       bytes_to_read);

		bytes_read += bytes_to_read;
		read_size -= bytes_to_read;
		block_offset++;
		block_offset_within_block = 0;
	}

	printf("[debug] Buffer content: %.*s\n",
	       (int) bytes_read,
	       buffer);  // Print the content of the buffer
	return bytes_read;
}


static int
fisopfs_createfiles(const char *path, mode_t mode, struct fuse_file_info *fi)
{
	printf("[debug] fisopfs_createfiles(%s)\n", path);
	return crear_archivo(path, mode, true);
}

static int
fisop_createdir(const char *path, mode_t mode)
{
	printf("[debug] fisop_createdir(%s) con modo: %d\n", path, mode);
	return crear_archivo(path, mode, false);
}


int
minimo(int a, int b)
{
	if (a < b)
		return a;
	return b;
}
static int
fisop_write(const char *path,
            const char *message,
            size_t size,
            off_t offset,
            struct fuse_file_info *fi)
{
	printf("[debug] fisop_write(%s)\n", path);
	int i = 0;
	int indice = search_inode_with_path(path);
	if (indice < 0) {
		// File not found, create the file
		int valor = crear_archivo(path, 0755, true);
		if (valor != 0) {
			// Failed to create file
			return valor;
		} else {
			indice = search_inode_with_path(path);
		}
	}

	// If the found item is a directory, throw an exception
	if (!inodes[indice].is_file) {
		errno = EISDIR;
		return -EISDIR;
	}

	if ((inodes[indice].mode & S_IWUSR) == 0) {
		errno = EACCES;
		return -EACCES;
	}

	inodes[indice].size = minimo(offset + size, BLOCKSIZE * DATABLOCKMAX);
	inodes[indice].date_of_modification = time(NULL);

	while (offset < (BLOCKSIZE * DATABLOCKMAX)) {
		if (!inodes[indice].is_free) {
			printf("Escribo:...\n");
			struct block *bloque =
			        &inodes[indice].data[offset / BLOCKSIZE];
			memcpy(bloque->content + (offset % BLOCKSIZE),
			       message,
			       minimo(size, BLOCKSIZE - offset % BLOCKSIZE));
			return minimo(size, BLOCKSIZE - offset % BLOCKSIZE);
		} else {
			if (buscar_bloque_libre(indice, offset / BLOCKSIZE) ==
			    ENOMEM)
				return i;
		}
	}

	if (offset + size > strlen(message)) {
		size = strlen(message) - offset;
	}
	size = size > 0 ? size : 0;
	printf("The message is %s\n", message);
	strncpy(inodes[indice].data[offset / BLOCKSIZE].content, message, size);
	// return the written bytes
	return size;
}


static int
fisop_removefile(const char *path)
{
	printf("[debug] fisop_removefile(%s)\n", path);

	int indice = search_inode_with_path(path);
	if (indice < 0) {  // no encontré el directorio
		errno = ENOENT;
		return -ENOENT;
	}

	// Si encuentro algo pero es un directorio, lanzo una excepción
	if (inodes[indice].is_file == false) {
		errno = EISDIR;
		return -EISDIR;
	}

	for (int j = 0; j < DATABLOCKMAX; j++) {
		if (inodes[indice].is_free == false) {
			printf("Borrando archivo...\n");
			// inodes[indice].data[j] = NULL;
			memset(data_blocks[j].content, 0, BLOCKSIZE);
			memset(&data_blocks[j], 0, sizeof(struct block));
			data_blocks[j].is_free = true;
		}
	}

	memset(&inodes[indice], 0, sizeof(struct inode));
	inodes[indice].is_free = true;

	// Actualizar las fechas de modificación y acceso
	inodes[indice].date_of_modification = time(0);
	inodes[indice].data_of_access = time(0);

	return 0;
}

// cuenta cuantas veces aparece la barra '/' en un string (ej. "/prueba/archivo.txt" -> 2)
int
contar(const char *nombre)
{
	int contador = 0;
	int i = 0;
	while (nombre[i] != '\0') {
		if (nombre[i] == '/')
			contador++;
		i++;
	}
	return contador;
}

static int
fisop_removedir(const char *path)
{
	printf("[debug] fisop_removedir(%s)\n", path);

	// buscar el directorio en los inodos
	int indice = search_inode_with_path(path);
	if (indice < 0) {  // no encontré el directorio
		errno = ENOENT;
		return -ENOENT;
	}

	// Si encuentro algo pero no es un directorio, lanzo una excepcion
	if (inodes[indice].is_file) {
		errno = ENOTDIR;
		return -ENOTDIR;
	}
	// Si el directorio no está vacio, elimino todos los archivos a mano y luego borro
	for (int i = 0; i < CANT_INODOS; i++) {
		if (!inodes[i].is_free && comp_str(path, inodes[i].name)) {
			if (inodes[i].is_file &&
			    contar(inodes[i].name) > contar(path))
				fisop_removefile(inodes[i].name);
			if (!inodes[i].is_file && strcmp(inodes[i].name, path) > 0)
				fisop_removedir(inodes[i].name);
		}
	}

	for (int j = 0; j < DATABLOCKMAX; j++) {
		if (inodes[indice].is_free == false) {
			printf("En rmdir:\n");
			// inodes[indice].data[j] = NULL;
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


void
update_file()
{
	FILE *file;
	file = fopen(archivo, "w");

	for (int i = 0; i < BLOCK_DATA_NUMBER; i++) {
		if (fwrite(&data_blocks[i], sizeof(struct block), 1, file) < 0) {
			printf("Write error\n");
			fclose(file);
			return;
		}
	}

	for (int i = 0; i < CANT_INODOS; i++) {
		if (fwrite(&inodes[i], sizeof(struct inode), 1, file) < 0) {
			printf("Write error\n");
			fclose(file);
			return;
		}
	}

	fclose(file);
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


// Busca la posición en el array de inodes, el inodo que tiene el name path
// importante: busca "/un_path" y no "un_path"
int
search_inode_with_path(const char *path)
{
	for (int i = 0; i < CANT_INODOS; i++) {
		if (&inodes[i] == NULL) {
			continue;
		}
		if (strcmp(inodes[i].name, path) == 0) {
			return i;
		}
	}
	return -1;
}


// crea un archivo (regular o directorio) inicializandolo y asignandole un inodo
// asi como su metadata correspondiente
int
crear_archivo(const char *path, mode_t mode, bool tipo)
{
	int inode_index = 0;
	int block_index = 0;

	// Buscar un bloque libre
	while (block_index < BLOCK_DATA_NUMBER &&
	       !data_blocks[block_index].is_free) {
		block_index++;
	}
	if (block_index >= BLOCK_DATA_NUMBER)
		return -ENOMEM;

	// Buscar un inodo libre
	while (inode_index < CANT_INODOS && !inodes[inode_index].is_free) {
		inode_index++;
	}
	if (inode_index >= CANT_INODOS)
		return -ENOMEM;

	// Marcarlo como ocupado
	data_blocks[block_index].is_free = false;  // inodo ocupado
	data_blocks[block_index].inode_index = inode_index;

	printf("data_blocks[block_index].inode_index = %d\n", inode_index);
	// Asignarle toda la info correspondiente
	struct inode archivo = inodes[inode_index];
	archivo.mode = mode;
	archivo.date_of_creation = time(NULL);
	archivo.data_of_access = time(NULL);
	archivo.date_of_modification = inodes[inode_index].date_of_creation;
	archivo.data[0] =
	        data_blocks[block_index];  // asigna el primer bloque al bloque libre encontrado


	archivo.st_uid = getuid();
	archivo.size = 0;
	archivo.is_file = tipo;
	archivo.is_free = false;
	strcpy(archivo.name, path);
	printf("aaaaaaaaaaaaaaaaaaaaaa\n");
	inodes[inode_index] = archivo;
	printf("El indice es %d\n", inodes[inode_index].data[0].inode_index);
	// Establecer los tiempos de acceso y modificación del archivo
	// update_time(path);

	return 0;
}


// cuenta cuantas veces aparece la barra '/' en un string (ej. "/prueba/archivo.txt" -> 2)
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


// buscar la posición de la barra que separa el name del directorio padre del
// name del archivo (ej. "/prueba/archivo.txt" -> 8)
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


// Compara dos strings, devuelve 0 si uno está contenido dentro del otro
// Y 1 si son diferentes
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


// inicializa el archivo donde se guardan los data (superbloque, inodes y
// bloques de data) y los carga en memoria
void
init_file()
{
	int valor = 0;
	FILE *file;
	struct inode *inodes_aux;
	struct block *info;
	inodes_aux = calloc(sizeof(struct inode), CANT_INODOS);
	info = calloc(sizeof(struct block), BLOCK_DATA_NUMBER);

	file = fopen(archivo, "w");
	if (file == NULL) {
		printf("Failed to open file\n");
		return;
	}


	for (int i = 0; i < BLOCK_DATA_NUMBER; i++) {
		// memset(&data_blocks[i], 0, sizeof(struct block));
		info[i].is_free = true;
		info[i].inode_index = -1;
		memcpy(&data_blocks[i], &info[i], sizeof(struct block));
	}

	for (int i = 0; i < CANT_INODOS; i++) {
		// memset(&inodes[i], 0, sizeof(struct inode));
		inodes_aux[i].is_free = true;
		memcpy(&inodes[i], &inodes_aux[i], sizeof(struct inode));
	}

	for (int i = 0; i < BLOCK_DATA_NUMBER; i++) {
		valor = fwrite(&info[i], sizeof(struct block), 1, file);
		if (valor < 0) {
			printf("Write error\n");
			fclose(file);
			return;
		}
	}

	for (int i = 0; i < CANT_INODOS; i++) {
		valor = fwrite(&inodes_aux[i], sizeof(struct inode), 1, file);
		if (valor < 0) {
			printf("Write error\n");
			fclose(file);
			return;
		}
	}

	free(inodes_aux);
	free(info);
	fclose(file);
}

// si el archivo donde se guardan los data en disco existe, lo carga en memoria
void
load_file_from_disk(FILE *file)
{
	int valor = 0;

	for (int i = 0; i < BLOCK_DATA_NUMBER; i++) {
		valor = fread(&data_blocks[i], sizeof(data_blocks[i]), 1, file);
		// print if datablocks is not free
		// if (!data_blocks[i].is_free)
		// 	printf("data_blocks[%d].is not free = %d\n",
		// 	       i,
		// 	       data_blocks[i].is_free);
		if (valor < 0) {
			printf("He fallado\n");
		}
	}

	for (int i = 0; i < CANT_INODOS; i++) {
		memset(&inodes[i].data, 0, sizeof(struct inode));
		valor = fread(&inodes[i], sizeof(inodes[i]), 1, file);
		if (valor < 0) {
			printf("He fallado\n");
		}
		// for (int j = 0; j < DATABLOCKMAX; j++) {
		// 	if (!inodes[i].is_free) {
		// 		int block_index = inodes[i].data[j].inode_index;
		// 		inodes[i].data[j] = data_blocks[block_index];
		// 	}
		// }
	}
}

// busca en el array de bloques de data un bloque libre y lo asigna al archivo
int
buscar_bloque_libre(int indice, int offset)
{
	int i = 0;
	while (i < BLOCK_DATA_NUMBER) {
		if (&data_blocks[i] != NULL) {
			inodes[indice].data[offset] = data_blocks[i];
			return 0;
		}
		i++;
	}
	return ENOMEM;
}


// dado un path actualiza la fecha de modificacion del inodo
void
update_time(const char *path)
{
	if (count_slash(path) > 1) {  // sino no estoy en el dir. raiz
		// busco el name del directorio padre (ej. "/prueba/archivo.txt" -> "/prueba")
		int indice_barra = search_slash(path) - 1;
		char name_directorio_padre[MAX_NAME];
		strncpy(name_directorio_padre, path, indice_barra);
		name_directorio_padre[indice_barra] = '\0';
		int index_dir_padre =
		        search_inode_with_path(name_directorio_padre);
		inodes[index_dir_padre].date_of_modification = time(NULL);
	}
}