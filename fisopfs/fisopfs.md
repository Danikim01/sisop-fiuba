# fisop-fs

# Diseño de nuestro filesystem:

El file system implementado consiste simplemente en un conjunto de 400 bloques de data, las cuales son referenciados por 100 inodos de forma que cada inodo tener hasta 4 referencias a bloques de datos.
El struct block es la que contiene el dato del archivo (o directorio) ademas de almacenar informacion sobre si la misma se encuentra libre o no. Algo similar ocurre con los inodos, las cuales son estructuras que sostienen la metadata correspondiente a cada archivo, y saben si se encuentran libre o no. De esta forma se evito usar bitmaps tanto para bloques de datos como bloques de inodos y de un superbloque.
Por su parte el inodo contiene muchos datos relevantes sobre el archivo al que hace referencia, como por ejemplo: el nombre, el tamaño del archivo, las fechas de acceso,
modificacion y creacion, uid, y si el archivo es un directorio o no.
La presencia de inodos posibilitó la creacion de archivos independientemente de si son archivos regulares o directorios, lo cual hizo que podamos crear múltiples niveles de directorios anidados entre sí.
Cabe aclarar que para la creacion de nuestro filesystem no se hizo uso de memoria dinamica sino de estructuras estaticas como vectores.

# Persistencia

Para la persistencia se opto por crear un archivo defecto ("myfs.fisopfs") en el cual se va a almacenar toda la informacion del filesystem. Cuando se inicia por primera vez el filesystem en la funcion fisop_init, se inicializan las estructuras correspondientes al filesystem y se almacenan en el archivo.
Si el archivo ya estaba creado, entonces el programa va a levantar el filsystem de disco haciendo una correspondiente deserializacion y de esa forma cargar todo a memoria para que pueda ser usada.
Es la funcion fisop_destroy la que va a actualizar constantemente el estado actual del filesystem y serializarlo.

# Aclaraciones extra

- Si se supera el espacio maximo del filesystem se arrojará el error: ENOSPC (ver la funcion create_file())
- La forma en la que se encuentra un archivo especificado dado un path lo hace la función search_inode_with_path(path) que recibe un path y devuelve la posición donde se encuentra el inodo correspondiente en el array de inodos.

# Casos de Test

Esta es la cantidad de niveles de directorios que soporta nuestro filesystem:

danikim@DESKTOP-0DG2U5K:~/sisop/sisop_2023a_g20/fisopfs/pruebas$ mkdir dir1
danikim@DESKTOP-0DG2U5K:~/sisop/sisop_2023a_g20/fisopfs/pruebas$ mkdir dir2
danikim@DESKTOP-0DG2U5K:~/sisop/sisop_2023a_g20/fisopfs/pruebas$ mkdir dir3
danikim@DESKTOP-0DG2U5K:~/sisop/sisop_2023a_g20/fisopfs/pruebas$ mkdir dir4
danikim@DESKTOP-0DG2U5K:~/sisop/sisop_2023a_g20/fisopfs/pruebas$ mkdir dir5
danikim@DESKTOP-0DG2U5K:~/sisop/sisop_2023a_g20/fisopfs/pruebas$ mkdir dir6
mkdir: cannot create directory ‘dir6’: No space left on device
danikim@DESKTOP-0DG2U5K:~/sisop/sisop_2023a_g20/fisopfs/pruebas$

Como se ve si se supera dicha cantidad devuelve un mensaje de error

Por otor lado aca se muestra que si el path es muy largo se tira un mensaje de error:

anikim@DESKTOP-0DG2U5K:~/sisop/sisop_2023a_g20/fisopfs/pruebas$ mkdir dir
danikim@DESKTOP-0DG2U5K:~/sisop/sisop_2023a_g20/fisopfs/pruebas$ ls
dir
danikim@DESKTOP-0DG2U5K:~/sisop/sisop_2023a_g20/fisopfs/pruebas$ cd dir
danikim@DESKTOP-0DG2U5K:~/sisop/sisop_2023a_g20/fisopfs/pruebas/dir$ mkdir dir2
danikim@DESKTOP-0DG2U5K:~/sisop/sisop_2023a_g20/fisopfs/pruebas/dir$ cd dir2/
danikim@DESKTOP-0DG2U5K:~/sisop/sisop_2023a_g20/fisopfs/pruebas/dir/dir2$ mkdir dir3
danikim@DESKTOP-0DG2U5K:~/sisop/sisop_2023a_g20/fisopfs/pruebas/dir/dir2$ cd dir3/
danikim@DESKTOP-0DG2U5K:~/sisop/sisop_2023a_g20/fisopfs/pruebas/dir/dir2/dir3$ ls
danikim@DESKTOP-0DG2U5K:~/sisop/sisop_2023a_g20/fisopfs/pruebas/dir/dir2/dir3$ mkdir dir4
mkdir: cannot create directory ‘dir4’: File name too long

Para probar esto la variable MAX_NAME 60 se ha modificado temporalmente a 20.
