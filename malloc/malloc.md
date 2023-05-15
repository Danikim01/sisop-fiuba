# malloc

## **Compilar los tests**

```
make clean && make -B -e USE_BF=true && make test

                       o

make clean && make -B -e USE_FF=true && make test

```

## **Parte 1**

En un principio no se presentó como necesario el uso de una abstracción bloque, se trabajó exclusivamente con regiones, siguiendo el apunte recomendado por la catedra: OSTEP, capítulo 17: Free-Space Management (PDF), pero con una pequeña modificación, en nuestra implementación la "free list" tambien contiene los bloques que han sido ocupados por el usuario, diferenciandose de los 
libres mediante un campo `bool free` que indica si la region está libre o no. Se hizo asi por simpleza
y ya que facilita el coalescing de las regiones.

## **Parte 2**

A la hora de tener multiples bloques de diferentes tamaños, para manejar la free list se decidió usar
la recomendación dada en la clase practica de manejar tres free list distintas, una para cada tamaño de
bloque. Se tienen multiples free list dado que la consigna indica que

```
A la hora de recibir un pedido de malloc(), la librería debería entonces iterar sobre todos los bloques que esté administrando, comenzando por los más pequeños hasta encontrar uno que posea una región apropiada.
```

En especial **comenzando por los más pequeños** indica la necesidad de orden. La otra opcion para adaptar la free list a multiples bloques, seria mantenerla ordenada (de menor a mayor), pero esto seria costoso no solo la hora de insertar un bloque (por ejemplo, si se tienen muchos bloques pequeños y se quiere insertar uno grande) sino tambien a la hora de buscarlo, por lo que se optó por la recomendación de la practica. 

Otro cambio que resultó necesario, fue la idea de introducir una abstración de bloque, para así identificar dónde terminaba y comenzaba el espacio de memoria que corresponde a un bloque (el cual anteriormente, al ser único, no era necesario). Para esto se decidió un analogo a la idea de región de una lista enlazada (que tiene una referencia a la primera region que le corresponde, para delimitar así bien cada bloque) y para poder trabajar con la estructura se decidió que el comienzo de cada bloque de memoria que se pedía llevaría al principio del mismo un struct bloque, por lo que en verdad, cada bloque solo tiene como maximo `BLOCK_SIZE - sizeof(struct region) - sizeof(struct block)` de memoria a utilizar, se decidió no pedir mas ya que es mínima la perdida respecto a la cantidad que se pide.

## **Parte 3**

Si bien durante la practica se dijo que solo hacia falta buscar en la lista del tamaño correspondiente a la memoria que se pedía para best fit, optamos por buscar en todas las listas, ya que lo dicho era por motivos de simplicidad y se nos hizo más correcto hacerlo de esta manera. 

## **Parte 4**

Para la redimension de memoria se tuvieron que tener en cuenta algunos casos especiales, como si al recibir un puntero a NULL la funcion de realloc es analoga a malloc, o que si el size es 0 la funcion realiza lo mismo que free. Luego tenemos otros dos casos generales: un achicamiento de memoria, donde pudimos reutilizar la funcion para dividir regiones de memoria, con la diferencia de que luego de realizar esto se setea la nueva region libre (la de la derecha) en cero; el otro caso general es el cual el size es superior al actual del bloque, se pueden dar dos casos, el primero es que la region contigua a la recibida este libre, y entre ambas sean capaces de alocar el nuevo tamaño, en tal caso se expande la region actual, tomando el tamaño necesario de la region a su "derecha", el otro caso es que la region actual no tenga una region libre a continuación, por lo que se realiza un malloc seguido de un memcopy, para finalmente liberar el puntero original y retormar el creado.