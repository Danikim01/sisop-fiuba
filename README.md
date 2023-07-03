
# Diseño de nuestro filesystem:

El file system implementado consiste simplemente en un conjunto de 100 bloques de data, las cuales son referenciados por 100 inodos de forma que cada inodo tiene referencia a un bloque de datos.
El struct block es la que contiene el dato del archivo (o directorio) ademas de almacenar informacion sobre si la misma se encuentra libre o no. El campo inode_index del bloque es el indice del inodo que apunta el bloque en cuestion, de forma que se pudo establecer un vinculo entre inodo-bloque dado que cada bloque sabe que inodo lo esta "apuntando" por asi decir.

Los inodos son estructuras que sostienen la metadata correspondiente a cada archivo, y al igual que los bloques saben si se encuentran libres o no. De esta forma se evito usar bitmaps tanto para bloques de datos como bloques de inodos.
Por su parte el inodo contiene muchos datos relevantes sobre el archivo al que hace referencia, como por ejemplo: el nombre, el tamaño del archivo, las fechas de acceso,
modificacion y creacion, uid, y si el archivo es un directorio o no, entre otros.
El campo inode_parent_index de los inodos sirve para fijarse si el archivo en cuestion, que en esencia es referenciado por un inodo, tiene referencias a un directorio padre, es decir, si esta contenido en un directorio. Esto fue especialmente util para el borrado de directorios dado que las mismas se tiene que borrar solo si se encuentran vacias.

A su vez, la presencia de inodos posibilitó la creacion de archivos independientemente de si son archivos regulares o directorios, lo cual hizo que podamos crear múltiples niveles de directorios anidados entre sí.
Cabe aclarar que para la creacion de nuestro filesystem no se hizo uso de memoria dinamica sino de estructuras estaticas como vectores.

# Persistencia

Para la persistencia se opto por crear un archivo defecto ("myfs.fisopfs") en el cual se va a almacenar toda la informacion del filesystem. Cuando se inicia por primera vez el filesystem en la funcion fisop_init, se inicializan las estructuras correspondientes al filesystem y se almacenan en el archivo.
Si el archivo ya estaba creado, entonces el programa va a levantar el filsystem de disco haciendo una correspondiente deserializacion y de esa forma cargar todo a memoria para que pueda ser usada.
Es la funcion fisop_destroy la que va a actualizar constantemente el estado actual del filesystem y serializarlo.

# Aclaraciones extra

- Si se supera el espacio maximo del filesystem se arrojará el error: ENOSPC (ver la funcion create_file())
- La forma en la que se encuentra un archivo especificado dado un path lo hace la función search_inode_with_path(path) que recibe un path y devuelve la posición donde se encuentra el inodo correspondiente en el array de inodos.

# Desafios realizados

## Primer desafio realizado:

- Soporte para múltiples directorios anidados

  - Más de dos niveles de directorios
  - Se debe implementar una cota máxima a los niveles de directorios y a la longitud del path

## Pruebas:

### Si el path es muy largo, el filesystem nos dira que es muy largo:

```bash
danikim@DESKTOP-0DG2U5K:~/sisop/sisop_2023a_g20/fisopfs/pruebas$ mkdir un_directorio_con_nombre_super_mega_largooooooooooooooooooooooooooooooooooooo
mkdir: cannot create directory ‘un_directorio_con_nombre_super_mega_largooooooooooooooooooooooooooooooooooooo’: File name too long
```

### Se ha definida una cota máxima a los niveles de directorios, de forma que si se crean mas de 4 niveles de directorios nuestro filsystem arrojara el siguiente error:

```bash
danikim@DESKTOP-0DG2U5K:~/sisop/sisop_2023a_g20/fisopfs/pruebas$ cd un_dir/
danikim@DESKTOP-0DG2U5K:~/sisop/sisop_2023a_g20/fisopfs/pruebas/un_dir$ mkdir dir2
danikim@DESKTOP-0DG2U5K:~/sisop/sisop_2023a_g20/fisopfs/pruebas/un_dir$ mkdir dir3
danikim@DESKTOP-0DG2U5K:~/sisop/sisop_2023a_g20/fisopfs/pruebas/un_dir$ cd dir3
danikim@DESKTOP-0DG2U5K:~/sisop/sisop_2023a_g20/fisopfs/pruebas/un_dir/dir3$ mkdir dir4
danikim@DESKTOP-0DG2U5K:~/sisop/sisop_2023a_g20/fisopfs/pruebas/un_dir/dir3$ cd dir4/
danikim@DESKTOP-0DG2U5K:~/sisop/sisop_2023a_g20/fisopfs/pruebas/un_dir/dir3/dir4$ mkdir dir5
danikim@DESKTOP-0DG2U5K:~/sisop/sisop_2023a_g20/fisopfs/pruebas/un_dir/dir3/dir4$ cd dir5/
danikim@DESKTOP-0DG2U5K:~/sisop/sisop_2023a_g20/fisopfs/pruebas/un_dir/dir3/dir4/dir5$ mkdir dir6
mkdir: cannot create directory ‘dir6’: Too many links
```

## Segundo desafio realizado

- Soporte para enlaces simbólicos
  - Debe implementarse la operación symlink
  - Deben incluirse pruebas utilizando link -s

## Pruebas y demostraciones (mediante link -s)

### Se crea un enlace simbolico correctamente

```bash
danikim@DESKTOP-0DG2U5K:~/sisop/sisop_2023a_g20/fisopfs/pruebas$ touch file.txt
danikim@DESKTOP-0DG2U5K:~/sisop/sisop_2023a_g20/fisopfs/pruebas$ ln -s file.txt enlace
danikim@DESKTOP-0DG2U5K:~/sisop/sisop_2023a_g20/fisopfs/pruebas$ ls -l
total 1
lrwxrwxrwx 1 danikim danikim 8 Jul 3 17:38 enlace -> file.txt
-rw-r--r-- 1 danikim danikim 0 Jul 3 17:38 file.txt
```

### Se lee el contenido que apunta el enlace simbolico correctamente en el mismo directorio

```bash
danikim@DESKTOP-0DG2U5K:~/sisop/sisop_2023a_g20/fisopfs/pruebas$ echo contenido > file.txt
danikim@DESKTOP-0DG2U5K:~/sisop/sisop_2023a_g20/fisopfs/pruebas$ cat file.txt
contenido
danikim@DESKTOP-0DG2U5K:~/sisop/sisop_2023a_g20/fisopfs/pruebas$ cat enlace
contenido
danikim@DESKTOP-0DG2U5K:~/sisop/sisop_2023a_g20/fisopfs/pruebas$ ls
enlace file.txt
```

### Se lee el contenido que apunta el enlace simbolico correctamente en otro directorio

```bash
danikim@DESKTOP-0DG2U5K:~/sisop/sisop_2023a_g20/fisopfs/pruebas$ mkdir otro_dir
danikim@DESKTOP-0DG2U5K:~/sisop/sisop_2023a_g20/fisopfs/pruebas$ cd otro_dir/
danikim@DESKTOP-0DG2U5K:~/sisop/sisop_2023a_g20/fisopfs/pruebas/otro_dir$ echo otro_contenido > file2.txt
danikim@DESKTOP-0DG2U5K:~/sisop/sisop_2023a_g20/fisopfs/pruebas/otro_dir$ ls
file2.txt
danikim@DESKTOP-0DG2U5K:~/sisop/sisop_2023a_g20/fisopfs/pruebas/otro_dir$ cd ..
danikim@DESKTOP-0DG2U5K:~/sisop/sisop_2023a_g20/fisopfs/pruebas$ ln -s otro_dir/file2.txt enlace2
danikim@DESKTOP-0DG2U5K:~/sisop/sisop_2023a_g20/fisopfs/pruebas$ cat enlace2
otro_contenido
```

### Stats del enlace simbolico

```bash
danikim@DESKTOP-0DG2U5K:~/sisop/sisop_2023a_g20/fisopfs/pruebas$ stat enlace2
  File: enlace2 -> otro_dir/file2.txt
  Size: 18              Blocks: 0          IO Block: 4096   symbolic link
Device: 57h/87d Inode: 6           Links: 1
Access: (0777/lrwxrwxrwx)  Uid: ( 1000/ danikim)   Gid: ( 1000/ danikim)
Access: 2023-07-03 17:40:26.000000000 -0300
Modify: 2023-07-03 17:40:26.000000000 -0300
Change: 2023-07-03 17:40:26.000000000 -0300
 Birth: -
```

### Si se borra el archivo apuntado por el enlace simbolico, leer el contenido apuntado arroja el error correcto

```bash
danikim@DESKTOP-0DG2U5K:~/sisop/sisop_2023a_g20/fisopfs/pruebas$ ls
enlace  enlace2  file.txt  otro_dir
danikim@DESKTOP-0DG2U5K:~/sisop/sisop_2023a_g20/fisopfs/pruebas$ rm file.txt
danikim@DESKTOP-0DG2U5K:~/sisop/sisop_2023a_g20/fisopfs/pruebas$ ls
enlace  enlace2  otro_dir
danikim@DESKTOP-0DG2U5K:~/sisop/sisop_2023a_g20/fisopfs/pruebas$ cat enlace
cat: enlace: No such file or directory
```

### Si creo un enlace simbólico con un nombre que ya existe en el directorio, arroja el error correcto

```bash
danikim@DESKTOP-0DG2U5K:~/sisop/sisop_2023a_g20/fisopfs/pruebas$ touch file3.txt
danikim@DESKTOP-0DG2U5K:~/sisop/sisop_2023a_g20/fisopfs/pruebas$ ln -s file3.txt enlace2
ln: failed to create symbolic link 'enlace2': File exists
```

