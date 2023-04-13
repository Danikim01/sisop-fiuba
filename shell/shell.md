# shell

### Búsqueda en $PATH

La familia de funciones `exec(3)`, se diferencian de la syscall `execve(2)`, ya que funcionan como wrappers de la misma, es decir, toda esta familia de funciones usa `execve(2)`, pero oculta complejidad extra que se ha añadido. Cada función de esta familia se caracteriza en base a las letras extra que tiene en su nombre, las cuales indican el comportamiento particular de cada una, se puede ver aquí que indica implica cada letra: https://stackoverflow.com/questions/20823371/what-is-the-difference-between-the-functions-of-the-exec-family-of-system-calls/47609180#47609180

### Procesos en segundo plano

Si, la familia de funciones `exec(3)` puede fallar, en caso de fallar retorna -1 y settea errno para que indique un error. En caso de fallo de cualquier funcion de `exec(3)` la shell simplemente imprime el error correspondiente, deja de ejecutar el código correspondiente a la ejecución de la instrucción actual y devuelve el command prompt.

### Flujo estándar

---

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
