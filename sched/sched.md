# TP3 : Scheduling

Lugar para respuestas en prosa, seguimientos con GDB y documentación del TP.

Para ejecutar el programa usando diferentes schedulers (ya sea el de Round Robin o el scheduler con prioridades) se debe modificar el flag CFLAG:

CFLAG += -D R_R para RR

CFLAG += -D C_P para scheduler con prioridades

# Explicacion de la implementacion de sched_yield() en sched.c:

Al llamar a sched_yield(), se incrementa el contador scheduler_calls para realizar un seguimiento del número de llamadas al scheduler.

Se guarda una referencia al entorno actual en la variable idle.

Luego, se establece el índice de inicio para buscar el siguiente entorno "runnable" en la lista de procesos libres. El índice se inicializa en 0, pero si hay un entorno actual (idle), se establece en el siguiente índice después del entorno actual.

A continuación, se ejecuta uno de los dos algoritmos de scheduling, dependiendo de las directivas de compilación:

a. Round Robin (R_R): El algoritmo de scheduling de round robin busca en la lista envs en un orden circular, comenzando desde el índice de inicio, hasta encontrar un entorno runnable. Si se encuentra uno, se llama a env_run() para ejecutarlo. Este algoritmo garantiza que todos los entornos runnable tengan una oportunidad de ejecución antes de que el ciclo se repita.

b. Scheduler con prioridades (C_P): La variable prioridad representa la prioridad global actual. Inicialmente se establece en 3.El scheduler se activa cuando se llama a la función sched_yield(). Se guarda el entorno actual (curenv) en la variable idle. Si el entorno actual (idle) no es nulo, se actualiza el índice de inicio para que comience desde el siguiente entorno después del entorno actual. Se declara una variable j e inicializa en 0. Esta variable se utilizará para realizar un seguimiento de la prioridad más alta encontrada durante la búsqueda de entornos ejecutables. Se llama a la función envs_execution_cp() dos veces: una vez para los entornos desde start_index hasta NENV y otra vez para los entornos desde 0 hasta start_index. Esta función se encarga de buscar los entornos ejecutables y determinar el entorno con la prioridad más alta.
Después de la ejecución de envs_execution_cp(), la variable j contiene la prioridad más alta encontrada. Se actualiza la variable prioridad con este valor. Tras lo cual, si el entorno actual (curenv) no es nulo y está en estado de ejecución y el número de CPU del entorno coincide con el número de CPU actual, se llama a la función env_run() para ejecutar el entorno actual. Finalmente, se llama a la función sched_halt(), que detiene el procesador y nunca retorna.

En resumen, el scheduler busca los entornos ejecutables con la prioridad más alta y los ejecuta. Después de cada llamada a sched_yield(), se actualiza la prioridad global con la prioridad más alta encontrada durante la búsqueda. Esto asegura que los entornos con prioridad más alta se ejecuten antes que los de prioridad más baja.

# Tests para la parte 3

Para testear el scheduler con prioridades se agregaron 6 procesos (dentro de kern/init.c). A cada uno se le seteo una determinada prioridad.
Se ejecuto sched_yield() para ver como se comporta el planificador.

```
qemu-system-i386 -serial mon:stdio -gdb tcp:127.0.0.1:26000 -D qemu.log -smp 1 -d guest_errors -kernel obj/kern/kernel
6828 decimal is 15254 octal!
Physical memory: 131072K available, base = 640K, extended = 130432K
check_page_free_list() succeeded!
check_page_alloc() succeeded!
check_page() succeeded!
check_kern_pgdir() succeeded!
check_page_free_list() succeeded!
check_page_installed_pgdir() succeeded!
SMP: CPU 0 found 1 CPU(s)
enabled interrupts: 1 2
[00000000] new env 00001000
0xf0232dac
[00000000] new env 00001001
0xf023a704
[00000000] new env 00001002
0xf024205c
[00000000] new env 00001003
0xf01253f4
[00000000] new env 00001004
0xf01253f4
[00000000] new env 00001005
0xf01253f4
hello, world
i am environment 00001003
my pid is 4099 and mi priority is 3
[00001003] exiting gracefully
[00001003] free env 00001003
hello, world
i am environment 00001004
my pid is 4100 and mi priority is 3
[00001004] exiting gracefully
[00001004] free env 00001004
hello, world
i am environment 00001005
my pid is 4101 and mi priority is 3
[00001005] exiting gracefully
[00001005] free env 00001005
Soy reduce priority 1,pid 4096 - La prioridad es: 2
[00001000] exiting gracefully
[00001000] free env 00001000
Soy reduce priority 2,pid 4097 - La prioridad es: 1
[00001001] exiting gracefully
[00001001] free env 00001001
Soy reduce priority 3,pid 4098 - La prioridad es: 1
[00001002] exiting gracefully
[00001002] free env 00001002
No runnable environments in the system!
Welcome to the JOS kernel monitor!
```

Se puede ver en el output que los procesos con mayor prioridad se ejecutan primero mientras que los procesos son menor prioridad se ejecutan mas tarde.

Las reducciones de prioridad solo suceden cuando usuario lo solicita mediante las syscalls sys_reduce_priority() o sys_set_priority() con dichas syscalls los programas de usuario podran modificar sus prioridades (entre 1 y 3).
Un proceso puede crear a otro proceso hijo mediante la syscall fork, la cual se ha implementado de forma que el proceso hijo herede la prioridad del padre y además se le decremente en una unidad la prioridad del padre.

Esto se puede ver en el output tras correr make run-pruebas-nox:

```
[00000000] new env 00001000
0xf02208a4
La PRIORIDAD de padre es: 3 su PID 4096
[00001000] new env 00001001
La nueva prioridad (reducida) del proceso 4096 es 2
[00001000] exiting gracefully
[00001000] free env 00001000

Mostrando estadisticas scheduler:

Llamadas al scheduler: 4
Cantidad de context switches:1
La PRIORIDAD PRIMER HIJO es: 3 su PID 4097
[00001001] new env 00002000
La nueva prioridad (reducida) del proceso 4097 es 2
[00001001] exiting gracefully
[00001001] free env 00001001

Mostrando estadisticas scheduler:

Llamadas al scheduler: 7
Cantidad de context switches:2
La PRIORIDAD NIETO: 3 su PID 8192
[00002000] exiting gracefully
[00002000] free env 00002000
No runnable environments in the system!
Welcome to the JOS kernel monitor!
Type 'help' for a list of commands.
```

En cuanto a las estadisticas que se quisieron mostrar:

- Cantidad de cambios de contexto
- Cantidad de llamadas al scheduler

# Seguimientos hechos en GDB

1 - Estado de los registros al inicio de la llamada a context_switch

![](/sched/Capturas/Imagen1.png)
![](/sched/Capturas/Imagen2.png)

2 - Estado del stack al inicio de la llamada a context_switch

![](/sched/Capturas/Imagen3.png)

3- Como cambia el stack a medida que avanzan las instrucciones
![](/sched/Capturas/Imagen4.png)
![](/sched/Capturas/Imagen5.png)

4 - Como se modifican los registros luego de ejecutar iret

![](/sched/Capturas/Imagen6.png)
