# fisop-fs

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

# Cosas a tener en cuenta

Si el path es muy largo, el filesystem nos dira que es muy largo:

```
danikim@DESKTOP-0DG2U5K:~/sisop/sisop_2023a_g20/fisopfs/pruebas$ mkdir un_directorio_con_nombre_super_mega_largooooooooooooooooooooooooooooooooooooo
mkdir: cannot create directory ‘un_directorio_con_nombre_super_mega_largooooooooooooooooooooooooooooooooooooo’: File name too long
```

Bajo esta optica, se pueden tener multiples niveles de directorios que dependen de cuan largo sea el path:

```
danikim@DESKTOP-0DG2U5K:~/sisop/sisop_2023a_g20/fisopfs/pruebas$ mkdir un_dirrrrrrrrr
danikim@DESKTOP-0DG2U5K:~/sisop/sisop_2023a_g20/fisopfs/pruebas$ cd un_dirrrrrrrrr/
danikim@DESKTOP-0DG2U5K:~/sisop/sisop_2023a_g20/fisopfs/pruebas/un_dirrrrrrrrr$ mkdir otrooooooooo_mas
danikim@DESKTOP-0DG2U5K:~/sisop/sisop_2023a_g20/fisopfs/pruebas/un_dirrrrrrrrr$ cd otrooooooooo_mas/
danikim@DESKTOP-0DG2U5K:~/sisop/sisop_2023a_g20/fisopfs/pruebas/un_dirrrrrrrrr/otrooooooooo_mas$ mkdir otroooooooooooooooooooooooooooooooooooo_mas
mkdir: cannot create directory ‘otroooooooooooooooooooooooooooooooooooo_mas’: File name too long
```
