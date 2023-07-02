# Desafios de shell


## *Pregunta:* Explicar detalladamente cómo se manejó la terminación del mismo.

La solucion consiste en settear un process group id a cada proceso en que corre en background, para esto se uso `setpgid(2)` (el numero de pgid elegido fue el pid de la shell), y en vez de esperar a que terminen los procesos que corren en background con un `waitpid(2)` en runcmd.c se utilizo un handler de la señal `SGCHLD` para el proceso de shell que lo que hace es liberar los recursos del proceso terminado con `waitpid(2)` (esperando solo a procesos del group id indicado para los procesos en segundo plano) e imprimir el mensaje indicado en consigna por consola.

## *Pregunta:* ¿Por qué es necesario el uso de señales?.

Es necesario el uso de señales porque es necesario avisar de la terminación de un proceso en segundo plano lo antes posible al usuario, la forma de hacer esto es que ni bien le llega al proceso padre (la shell) la señar correspondiente a un proceso hijo (`SIGCHLD`), se le indique al proceso padre como manejarla, que en este caso seria, liberando los recursos del mismo con `waitpid()` e imprimiendo lo pedido por consigna.

