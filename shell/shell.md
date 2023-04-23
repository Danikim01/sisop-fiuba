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

**Pregunta:**  Investigar qué ocurre con el exit code reportado por la shell si se ejecuta un pipe.

En el archivo printstatus.c se ve que si el tipo del commando parseado es pipe, entonces la funcion no imprime nada, por lo tanto no se imprime por pantalla si un proceso de tipo pipe termino. Se puede ver en el siguiente ejemplo:

```
 (/home/nehuen) 
$ ls | grep a | wc -w
5
 (/home/nehuen) 
$ 
```

Se ve que no cambia en nada en la terminal normal:

```
~$ ls | grep a | wc -w
5
~$ 
```

Y si alguno de los commandos falla, el comportamiento tambien es igual en ambas, en nuestra shell:

```
 (/home/nehuen) 
$ ls /noexiste | grep "a"
ls: cannot access '/noexiste': No such file or directory
 (/home/nehuen) 
$ 
```

Y en la terminal normal:

```
~$ ls /noexiste | grep "a"
ls: cannot access '/noexiste': No such file or directory
~$ 
```

### Variables de entorno temporarias


Responder: ¿Por qué es necesario hacerlo luego de la llamada a fork(2)?

Respuesta : Es necesario hacerlo después de la llamada a fork(2) porque cualquier cambio en las variables de entorno antes de fork(2) afectaría tanto al proceso hijo como al proceso padre. Si se hace después de fork(2), solo afectará al proceso hijo.

Responder: En algunos de los wrappers de la familia de funciones de exec(3) (las que finalizan con la letra e), se les puede pasar un tercer argumento (o una lista de argumentos dependiendo del caso), con nuevas variables de entorno para la ejecución de ese proceso. Supongamos, entonces, que en vez de utilizar setenv(3) por cada una de las variables, se guardan en un arreglo y se lo coloca en el tercer argumento de una de las funciones de exec(3).
¿El comportamiento resultante es el mismo que en el primer caso? Explicar qué sucede y por qué.
Describir brevemente (sin implementar) una posible implementación para que el comportamiento sea el mismo.

No, el comportamiento no es el mismo. Cuando se utilizan las funciones de exec(3) para pasar una lista de nuevas variables de entorno, estas variables reemplazan todas las variables de entorno existentes en el proceso hijo. Por lo tanto, si se pasa una lista con solo las nuevas variables de entorno, se perderán todas las variables de entorno existentes. En cambio, si se utiliza setenv(3) para agregar nuevas variables de entorno, se mantendrán todas las variables de entorno existentes y se agregarán las nuevas.

Para implementar esta opción, se podría utilizar alguna de las funciones de la familia exec(3) que acepte un arreglo de variables de entorno como tercer parámetro, en lugar de utilizar setenv(3) para cada variable nueva. Para ello, se podría crear un arreglo de strings que contenga las variables de entorno a agregar, y luego pasarlo como tercer parámetro en la función exec correspondiente. Por ejemplo, la función execlpe(3) acepta un arreglo de strings como tercer parámetro para las variables de entorno.


---

### Pseudo-variables

Algunas otras variables mágicas estándar y su propósito son:

1.
Ejemplo de uso en bash:

$ echo $$
19989

2.
!: Contiene el número de historia del comando anterior. Es útil para repetir comandos anteriores o modificarlos.
Ejemplo de uso en bash:

$ !ls

3.
$_: Contiene el último argumento del comando anterior. Es útil para reutilizar el último argumento en otro comando.
Ejemplo de uso en bash:

$ ls /ruta/a/mi/carpeta
$ cd $_

Esto cambiará al directorio "/ruta/a/mi/carpeta" sin tener que escribir la ruta completa de nuevo.

---

### Comandos built-in

Responder: ¿Entre cd y pwd, alguno de los dos se podría implementar sin necesidad de ser built-in? ¿Por qué? ¿Si la respuesta es sí, cuál es el motivo, entonces, de hacerlo como built-in? (para esta última pregunta pensar en los built-in como true y false)

Un comando built-in es un comando integrado en la propia shell y que no requiere la creación de un nuevo proceso para su ejecución, es decir, se ejecuta directamente dentro de la shell. Estos comandos son parte del intérprete de comandos y, por lo tanto, tienen un acceso directo y rápido a los recursos del sistema y a las variables internas de la shell.

Ahora bien, en el caso de cd, este comando debe ser implementado como built-in ya que su función es cambiar el directorio actual de trabajo del proceso que lo ejecuta y, si fuera implementado como un programa externo, sólo cambiaría el directorio del nuevo proceso que se crea para su ejecución, sin afectar al proceso de la shell que lo llama.

Por otro lado, en el caso de pwd, que muestra el directorio actual de trabajo, es un comando que puede ser implementado como un programa externo ya que sólo lee el directorio actual de trabajo del proceso y no lo modifica. En ese sentido, pwd no requiere tener acceso directo a los recursos del sistema y puede ser ejecutado como un programa externo. Sin embargo, implementar pwd como built-in tiene algunas ventajas, como el hecho de que se puede acceder más rápidamente a la información del directorio actual sin necesidad de crear un nuevo proceso.


---

### Historial

En el modo no canónico, al leer la entrada desde la terminal, los parámetros MIN y TIME de la llamada al sistema select() se utilizan para especificar el número mínimo de caracteres a leer y el tiempo máximo de espera para la entrada antes de que se devuelva.

El parámetro MIN especifica el número mínimo de caracteres que deben leerse antes de que select() devuelva. El parámetro TIME especifica el tiempo máximo de espera para la entrada en décimas de segundo. Si el parámetro TIME se establece en 0, select() devolverá inmediatamente. Si TIME se establece en NULL, select() bloqueará indefinidamente hasta que se hayan leído los MIN caracteres.

Juntos, estos parámetros permiten que un programa lea la entrada desde la terminal de manera no bloqueante, con un tiempo de espera. Esto puede ser útil en escenarios donde el programa necesita leer la entrada del usuario, pero también necesita realizar otras tareas mientras espera la entrada.

---
