# Planificador dieciochero

Tarea 1 de Sistemas Operativos (UDP). Nuestro programa lee un archivo .txt, que es un plan de actividades, cada una
con sus respectivas dependencias y lo ejecuta como un DAG usando procesos, pipes y señales.

No se usan threads ni mecanismos de sincronización de hilos. La coordinación
se hace con `fork()`, `wait()`, pipes y señales.

## Archivos

- `Tarea1SistemasOperativos.c`: programa principal.
- `generador_estres.c`: genera un plan grande para probar el criterio 2.4, para simular previo a la evaluación.
- `plan.txt`: ejemplo de entrada.

- `plan.txt`: ejemplo de entrada.

Dejamos casi toda la lógica en un solo archivo (`Tarea1SistemasOperativos.c`)
en vez de separar en varios .c/.h. Con el tamaño real del programa (un main,
un par de funciones auxiliares como `trim()`, `liberar_espacio()` y
`abortar_dependientes()`, y el struct `datoken`) separar en módulos no
aportaba nada, solo más archivos para ir a buscar algo que está a dos
scrolls de distancia. Lo que sí separamos es lo que tenía sentido separar
de verdad: `generador_estres.c` es un programa aparte porque se ejecuta
aparte, antes y de forma independiente del planificador (genera el
`plan_estres.txt` que después el planificador simplemente lee como
cualquier otro plan), no porque comparta código con él.
 
 
## Compilación

El programa principal se compila con:

```bash
gcc -Wall -Wextra -std=c17 Tarea1SistemasOperativos.c -o planificador
```

Para `-lpthread`; no se necesita porque este programa no usa
threads ni ninguna API de hilos. En caso de querer compilar exactamente con esa
flag también funciona:

```bash
gcc -Wall -Wextra -std=c17 Tarea1SistemasOperativos.c -lpthread -o planificador
```

El generador:

```bash
gcc -Wall -Wextra -std=c17 generador_estres.c -o generador_estres
```

## Uso

```bash
./planificador plan.txt K
```

Por ejemplo:

```bash
./planificador plan.txt 2
```

El formato de cada actividad es:

```text
ID : nombre : tiempo_ms : dependencias
```

Si el tiempo viene vacío, el programa genera uno aleatorio entre 100 y 5000
ms. Las dependencias se separan por comas.

Definimos _POSIX_C_SOURCE 200809L antes de cualquier #include, porque funciones
como strtok_r(), fork(), wait(), pipe() y nanosleep() son extensiones POSIX, no
parte del estándar C17 puro que exige -std=c17. Sin esa macro, el compilador las 
trata como no declaradas, no funciona.

## 1.1 Parseo

Se usa `getline()` para no depender de un buffer de tamaño fijo. Después se
usa `strtok_r()` para separar los campos por `:` y otro `strtok_r()` para las
dependencias, se usa esta versión y no `strtok()` porque para las dependencias
necesitamos dos tokenizaciones anidadas, y al estar las dos usando el mismo punto
de guardado al mismo tiempo se "pisaban" entre ellas.

Los IDs se guardan como strings porque la tarea permite IDs alfanuméricos.
`trim()` elimina los espacios que vienen alrededor de los campos.

Cuando el tiempo está vacío, se usa:

```c
100 + rand() % 4901
```

para obtener un tiempo random entre 100 y 5000 ms.

## 1.2 DAG

Cada actividad guarda sus dependencias en `dependencias`. Además se arma
`dlist`, que es la lista inversa: si una actividad A es dependencia de B,
A guarda un puntero hacia B.

Para encontrar los IDs se usa `strcmp()` recorriendo las actividades. Esto es
O(n²), pero nos dejó trabajar con IDs alfanuméricos sin agregar una
estructura más complicada como por ejemplo implementar un map desde 0.

Una dependencia que no existe en el archivo se considera un plan mal formado
y el programa termina con error.

También se revisan ciclos. Si no hay ninguna raíz al principio, se informa el
posible ciclo. Si hay raíces pero después quedan actividades sin resolver,
se informa como ciclo parcial.

Se guardan punteros directos a struct datoken en dlist, para que al avisar a los dependientes
en tiempo de ejecución no haya que volver a buscar con strcmp, solo sucede una vez.

Se mantienen dos contadores separados: nd cantidad total de dependencias y restantes, cuántas faltan
para terminar. Si se usara un solo campo para ambos, cuando llegue a 0 perdemos el número original de
dependencias.

## 2.1 Procesos y límite K

Cada actividad que queda lista se ejecuta en un proceso hijo creado con
`fork()`.

La cola `q` guarda las actividades que ya pueden comenzar. El padre mantiene
`activos` y no crea más hijos cuando llega a `K`.

Cuando se llega al límite se usa `wait()` para esperar a que termine cualquiera 
de los hijos. No se hace busy-waiting.

## 2.2 Pipes

Cada actividad tiene su propio pipe, creado justo antes de hacer `fork()`.

El hijo escribe un mensaje corto antes de terminar:

```text
nombre:listo
```

o:

```text
nombre:fallo
```

El padre hace `wait()` y después lee el mensaje del pipe. Así no se dejan
miles de pipes abiertos al mismo tiempo cuando se prueba con muchas
actividades.

## 2.3 Aislamiento de errores

Cada hijo tiene una probabilidad de 1/10 de fallar. Esto se hace para poder
probar el aislamiento de errores, ya que el enunciado no define un mecanismo
concreto para simular fallos.

Si un hijo termina con código distinto de cero, su actividad queda marcada
como fallida. Sus dependientes reciben el mismo estado y no se hace `fork()`
para ellos. El fallo se propaga por `dlist`.

Basta con que una sola dependencia falle para que toda la cadena de sus dependientes
quede marcada como fallida, incluso si otras dependencias de esos mismos nodos terminaron bien.

El hijo vuelve a sembrar `rand()` usando `time(NULL) ^ getpid()` porque
`fork()` copia el estado del generador aleatorio. Esto tiene que estar,
si no, varios hijos creados casi al mismo tiempo podrían terminar usando 
la misma secuencia.

## 2.4 Prueba de estrés

`generador_estres.c` recibe la cantidad de actividades:

```bash
./generador_estres 10000
```

y crea `plan_estres.txt`.

Cada actividad depende de 0 a 3 actividades anteriores. Como nunca depende de
una actividad posterior, el grafo generado no puede tener ciclos.

El pipe se crea solo cuando una actividad va a ejecutarse, no para todas de
una vez. Esto evita tener miles de file descriptors abiertos simultáneamente.

## SIGINT

Se instala un manejador con `sigaction()` antes de crear procesos.

El manejador solamente cambia una bandera `volatile sig_atomic_t`. No hace
`printf()`, `malloc()` ni otras operaciones dentro de la señal.

No se usa `SA_RESTART` a propósito: así `wait()` puede terminar con `EINTR`
cuando llega Ctrl+C.

Cuando llega SIGINT, el padre manda `SIGTERM` a los hijos activos y espera por
ellos con `waitpid()` antes de terminar, evitando dejar zombies o hijos
huérfanos.

Cuando a wait() lo interrumpe la signal errno == EINTR, se corta el ciclo principal
en vez de reintentar el wait(), para pasar de inmediato a la limpieza de los procesos
que están activos.

## Memoria

Al terminar normalmente se libera la estructura principal, los strings, las
dependencias y las listas inversas. También se liberan en los caminos de
error relevantes.

## Importante

No hay `pthread_create`, `pthread_join`, mutex, condition variables,
semaphores de hilos ni ninguna otra herramienta de threads. La concurrencia
del trabajo se implementa exclusivamente con procesos.
