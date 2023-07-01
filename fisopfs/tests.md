`ls` sobre un directorio de pruebas:
```bash
$ cd ./test_dir # test_dir no posee contenido inicialmente
/test_dir$ ls
/test_dir$ 
```

`touch` para crear un archivo nuevo:
```bash
/test_dir$ touch new_file.dat
/test_dir$ ls # test_dir ahora contiene new_file.dat
new_file.dat
/test_dir$ 
```

`cat` del nuevo archivo no deberia poseer contenido:
```bash
/test_dir$ cat new_file.dat
/test_dir$ 
```

`echo >` nos crea un archivo con contenido:
```bash
/test_dir$ echo foo > foo.file # creamos un archivo con 'foo' como unico contenido
/test_dir$ cat foo.file
foo # contenido del archivo
/test_dir$
```

`cat` de un archivo inexistente deberia fallar:
```bash
/test_dir$ cat non_existant.file
cat: non_existant.dat: No such file or directory
/test_dir$ 
```


`mkdir` sobre el directorio actual crea un nuevo subdirectorio:
```bash
/test_dir$ mkdir dirA
/test_dir$ cd dirA
/test_dir/dirA$ mkdir dirB
/test_dir/dirA$ ls # ls sobre test_dir
dirB
/test_dir/dirA$
```

`ls -R` sobre directorio `test_dir` nos muestra los subdirectorios: 
```bash
/test_dir$ ls -R
.:
dirA  foo.file  new_file.dat

./dirA:
dirB

./dirA/dirB:
/test_dir$ 
```

`ls -l` nos muestra los detalles de los archivos del directorio:
```bash
/test_dir$ ls -l
total 4
drwxr-xr-x 2 juan518 juan518 0 Jul  1 19:28 dirA
-rw-r--r-- 1 juan518 juan518 4 Jul  1 19:24 foo.file
-rw-r--r-- 1 juan518 juan518 0 Jul  1 19:20 new_file.dat
/test_dir$ 
```

`rm` de un archivo inexistente falla:
```bash
/test_dir$ ls
dirA  foo.file  new_file.dat
/test_dir$ rm non_existant.file
rm: cannot remove 'non_existant.file': No such file or directory
/test_dir$
```

`rm` de un archivo existente no falla:
```bash
/test_dir$ ls
dirA  foo.file  new_file.dat
/test_dir$ rm foo.file
/test_dir$ ls
dirA  new_file.dat # foo.file ya no existe
/test_dir$ 
```

`rmdir` sobre directorio no vacio falla:
```bash
/test_dir$ rmdir dirA
rmdir: failed to remove 'dirA': Directory not empty
/test_dir$
```

`rmdir` sobre directorio vacio no falla:
```bash
/test_dir$ cd dirA
/test_dir/dirA$ ls -R
.:
dirB

./dirB:
/test_dir/dirA$ rmdir dirB
/test_dir/dirA$ ls # ya no devuelve contenido
/test_dir/dirA$ 
```

`stat` de un directorio devuelve información sobre el mismo:
```bash
/test_dir$ stat dirA
  File: dirA
  Size: 0               Blocks: 0          IO Block: 4096   directory
Device: 4bh/75d Inode: 6           Links: 2
Access: (0755/drwxr-xr-x)  Uid: ( 1000/ juan518)   Gid: ( 1000/ juan518)
Access: 2023-07-01 19:28:31.000000000 -0300
Modify: 2023-07-01 19:40:07.000000000 -0300
Change: 2023-07-01 19:40:07.000000000 -0300
 Birth: 2023-07-01 19:22:19.000000000 -0300
/test_dir$ 
```

