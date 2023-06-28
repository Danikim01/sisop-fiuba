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
#define DATABLOCKMAX 4   // cantidad de bloques de data que puede tener un inodo
#define BLOCKSIZE 4096   // año de cada bloque
#define MAX_name 60      // largo de name de archivo
#define MAX_name_DIR 55  // largo del path de un directorio
#define OCUPADO 1
#define LIBRE 0
#define DIR_T 0  // tipo de archivo directorio
#define REG_T 1  // tipo de archivo regular


struct block {
	char content[BLOCKSIZE];
};

struct inode {
	off_t size;
	mode_t mode;  // mode => rwx(owner)r-x(group)r-x(other) con r=read, w=write, x=execute
	time_t date_of_creation;
	time_t date_of_modification;
	time_t data_of_access;
	char name[MAX_name];
	uid_t st_uid;
	// int indice_bloques[DATABLOCKMAX];  // id de cada bloque de data, si es negativo no se usa
	struct block *data[DATABLOCKMAX];  // apunta a los bloques de data (un
	                                   // inodo puede tener mas de un bloque de data)
	bool is_file;  // 0 (false) es directorio, 1 (true) es archivo regular
};

struct superblock {
	int inode_amount;
	int data_blocks_amount;
	int bitmap_data[BLOCK_DATA_NUMBER];
	int bitmap_inodes[CANT_INODOS];
};


struct superblock SB;
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
			if (SB.bitmap_inodes[i] == OCUPADO &&
			    count_slash(inodes[i].name) == 1)
				filler(buffer, inodes[i].name + 1, NULL, 0);
		}
		return 0;
	}
	int barritas = count_slash(path);
	for (int i = 0; i < CANT_INODOS; i++) {
		if (SB.bitmap_inodes[i] == OCUPADO &&
		    comp_str(path, inodes[i].name))
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
fisopfs_read(const char *path,
             char *buffer,
             size_t size,
             off_t offset,
             struct fuse_file_info *fi)
{
	printf("[debug] fisopfs_read(%s, %lu, %lu)\n", path, offset, size);

	return 0;  // Opcional, dependiendo de la implementación
}


static int
fisopfs_createfiles(const char *path, mode_t mode, struct fuse_file_info *fi)
{
	printf("[debug] fisopfs_createfiles(%s)\n", path);
	if (strlen(path) > (MAX_name)) {
		errno = ENAMETOOLONG;
		return ENAMETOOLONG;
	}
	int valor = crear_archivo(path, mode, REG_T);
	return valor;
}

static int
fisop_createdir(const char *path, mode_t mode)
{
	printf("[debug] fisop_createdir(%s) con modo: %d\n", path, mode);
	if (strlen(path) >
	    (MAX_name_DIR)) {  // comparo con MAX_name-5 para que quede espacio
		               // para el path de un archivo que cree adentro
		errno = ENAMETOOLONG;
		return ENAMETOOLONG;
	}
	int valor = crear_archivo(path, mode, DIR_T);
	return valor;
}

static struct fuse_operations operations = {
	.create = fisopfs_createfiles,
	.getattr = fisopfs_getattr,
	.init = fisop_init,
	.mkdir = fisop_createdir,
	.read = fisopfs_read,
	.readdir = fisopfs_readdir,
	//.unlink = fisop_removefile,
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
		if (SB.bitmap_inodes[i] == LIBRE) {
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

	// buscar un bloque libre
	while (block_index < BLOCK_DATA_NUMBER &&
	       SB.bitmap_data[block_index] == OCUPADO) {
		block_index++;
	}
	if (block_index >= BLOCK_DATA_NUMBER)
		return -ENOMEM;

	// Buscar un inodo libre
	while (inode_index < CANT_INODOS &&
	       SB.bitmap_inodes[inode_index] == OCUPADO) {
		inode_index++;
	}
	if (inode_index >= CANT_INODOS)
		return -ENOMEM;

	// marcarlo como ocupado
	SB.bitmap_inodes[inode_index] = OCUPADO;  // inodo ocupado
	SB.bitmap_data[block_index] = OCUPADO;    // data ocupado

	// Asignarle toda la info correspondiente
	struct inode archivo = inodes[inode_index];
	archivo.mode = mode;
	archivo.date_of_creation = time(NULL);
	archivo.data_of_access = time(NULL);
	archivo.date_of_modification = inodes[inode_index].date_of_modification;
	archivo.data[0] =
	        &data_blocks[block_index];  // asigna el primer bloque al bloque libre encontrado
	archivo.st_uid = getuid();
	archivo.size = 0;
	archivo.is_file = tipo;

	strcpy(archivo.name, path);
	inodes[inode_index] = archivo;
	// Establecer los tiempos de acceso y modificación del archivo
	update_time(path);

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

void
create_sb()
{
	SB.data_blocks_amount = BLOCK_DATA_NUMBER;
	SB.inode_amount = CANT_INODOS;
	for (int i = 0; i < BLOCK_DATA_NUMBER; i++) {
		SB.bitmap_data[i] = 0;
	}
	for (int i = 0; i < CANT_INODOS; i++) {
		SB.bitmap_inodes[i] = 0;
	}
}

// inicializa el archivo donde se guardan los data (superbloque, inodes y
// bloques de data) y los carga en memoria
void
init_file()
{
	create_sb();
	int valor = 0;
	struct superblock sb;
	struct inode inodes[CANT_INODOS];
	struct block info[BLOCK_DATA_NUMBER];
	FILE *file;

	file = fopen(archivo, "w");
	if (file == NULL) {
		printf("Failed to open file\n");
		return;
	}

	memset(&sb, 0, sizeof(struct superblock));
	sb.data_blocks_amount = BLOCK_DATA_NUMBER;
	sb.inode_amount = CANT_INODOS;

	valor = fwrite(&sb, sizeof(struct superblock), 1, file);
	if (valor < 0) {
		printf("Write error\n");
		fclose(file);
		return;
	}

	for (int i = 0; i < BLOCK_DATA_NUMBER; i++) {
		valor = fwrite(&sb.bitmap_data[i], sizeof(int), 1, file);
		if (valor < 0) {
			printf("Write error\n");
			fclose(file);
			return;
		}
	}

	for (int i = 0; i < CANT_INODOS; i++) {
		valor = fwrite(&sb.bitmap_inodes[i], sizeof(int), 1, file);
		if (valor < 0) {
			printf("Write error\n");
			fclose(file);
			return;
		}
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
		valor = fwrite(&inodes[i], sizeof(struct inode), 1, file);
		if (valor < 0) {
			printf("Write error\n");
			fclose(file);
			return;
		}
	}

	fclose(file);
}

// si el archivo donde se guardan los data en disco existe, lo carga en memoria
void
load_file_from_disk(FILE *file)
{
	int valor = 0;
	valor = fread(&SB, sizeof(SB), 1, file);
	if (valor < 0) {
		printf("He fallado\n");
	}

	for (int i = 0; i < BLOCK_DATA_NUMBER; i++) {
		valor = fread(&SB.bitmap_data[i], sizeof(SB.bitmap_data[i]), 1, file);
		if (valor < 0) {
			printf("He fallado\n");
		}
	}
	for (int i = 0; i < CANT_INODOS; i++) {
		valor = fread(&SB.bitmap_inodes[i],
		              sizeof(SB.bitmap_inodes[i]),
		              1,
		              file);
		if (valor < 0) {
			printf("He fallado\n");
		}
	}

	for (int i = 0; i < BLOCK_DATA_NUMBER; i++) {
		valor = fread(&data_blocks[i], sizeof(data_blocks[i]), 1, file);
		if (valor < 0) {
			printf("He fallado\n");
		}
	}

	for (int i = 0; i < CANT_INODOS; i++) {
		valor = fread(&inodes[i], sizeof(inodes[i]), 1, file);
		if (valor < 0) {
			printf("He fallado\n");
		}
	}
}


// busca en el array de bloques de data un bloque libre y lo asigna al archivo
int
buscar_bloque_libre(int indice, int offset)
{
	int i = 0;
	while (i < BLOCK_DATA_NUMBER) {
		if (SB.bitmap_data[i] == LIBRE) {
			SB.bitmap_data[i] = OCUPADO;
			inodes[indice].data[offset] = &data_blocks[i];
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
		char name_directorio_padre[MAX_name];
		strncpy(name_directorio_padre, path, indice_barra);
		name_directorio_padre[indice_barra] = '\0';
		int index_dir_padre =
		        search_inode_with_path(name_directorio_padre);
		inodes[index_dir_padre].date_of_modification = time(NULL);
	}
}