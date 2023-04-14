# shell

### Búsqueda en $PATH

**Pregunta:** ¿cuáles son las diferencias entre la syscall execve(2) y la familia de wrappers proporcionados por la librería estándar de C (libc) exec(3)?

La familia de funciones `exec(3)`, se diferencian de la syscall `execve(2)`, ya que funcionan como wrappers de la misma, es decir, toda esta familia de funciones usa `execve(2)`, pero oculta complejidad extra que se ha añadido. Cada función de esta familia se caracteriza en base a las letras extra que tiene en su nombre, las cuales indican el comportamiento particular de cada una, se puede ver aquí que indica implica cada letra: https://stackoverflow.com/questions/20823371/what-is-the-difference-between-the-functions-of-the-exec-family-of-system-calls/47609180#47609180

**Pregunta:** ¿Puede la llamada a exec(3) fallar? ¿Cómo se comporta la implementación de la shell en ese caso?

Si, la familia de funciones `exec(3)` puede fallar, en caso de fallar retorna -1 y settea errno para que indique un error. En caso de fallo de cualquier funcion de `exec(3)` la shell simplemente imprime el error correspondiente, deja de ejecutar el código correspondiente a la ejecución de la instrucción actual y devuelve el command prompt.

### Procesos en segundo plano

**Pregunta:** Detallar cuál es el mecanismo utilizado para implementar procesos en segundo plano.

Si se detecta que un comando tiene que se ejecutado en segundo plano, lo que se hace es simplemente ejecutarlo y no hacerle wait dentro del caso que corresponde en exec.c. Pero, para que no queden procesos huerfanos, lo que se hace es, como indica la consigna, oportunamente antes de devolver cada command prompt, ver si algún proceso hijo del proceso padre (el de la shell), terminó.

### Flujo estándar

**Pregunta:** 
1 - Investigar el significado de 2>&1, explicar cómo funciona su forma general
2 - Mostrar qué sucede con la salida de cat out.txt en el ejemplo.
3 - Luego repetirlo, invirtiendo el orden de las redirecciones (es decir, 2>&1 >out.txt). ¿Cambió algo? Compararlo con el comportamiento en bash(1)

1 - 2>&1 Redirige stderr a stdout
2- Ocurre lo siguiente:
```
~$ ls -C /home /noexiste >out.txt 2>&1
~$ cat out.txt
ls: cannot access '/noexiste': No such file or directory
/home:
lost+found  nehuen
```
3 - 
En nuestra terminal, ocurre lo siguiente:

```
 (/home/nehuen) 
$ rm out.txt
        Program: [rm out.txt] exited, status: 0 
 (/home/nehuen) 
$ ls /home /noexiste 2>&1 >out.txt
        Program: [ls /home /noexiste 2>&1 >out.txt] exited, status: 2 
 (/home/nehuen) 
$ cat out.txt
ls: cannot access '/noexiste': No such file or directory
/home:
lost+found
nehuen
```

Como se redirige el stderr al stdout antes de hacer `>out.txt` entonces lo que ocurre es que se redirige a stdout la salida de stderr de haber hecho `ls /noexiste`, y tanto la salida de haber hecho `ls /home` como la de `ls /noexiste` van a `out.txt` ya que se redirigió el stdout del comando `ls /home /noexiste` a ese archivo habiendo hecho `>out.txt`.


En cambio, ocurre lo siguiente en `bash(1)`:

```
~$ ls -C /home /noexiste 2>&1 >out.txt
ls: cannot access '/noexiste': No such file or directory
~$ cat out.txt
/home:
lost+found  nehuen
```

### Tuberías múltiples

---

### Variables de entorno temporarias

---

### Pseudo-variables

---

### Comandos built-in

---

### Historial

---
