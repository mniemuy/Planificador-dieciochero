# Planificador dieciochero

Tarea 1 de Sistemas Operativos (UDP). El programa lee un plan de actividades
con dependencias y lo ejecuta como un DAG usando procesos, pipes y señales.

No se usan threads ni mecanismos de sincronización de hilos. La coordinación
se hace con `fork()`, `wait()`, pipes y señales.

## Archivos

- `Tarea1SistemasOperativos.c`: programa principal.
- `generador_estres.c`: genera un plan grande para probar el criterio 2.4.
- `plan.txt`: ejemplo de entrada.

## Compilación

El programa principal se compila con:

```bash
gcc -Wall -Wextra -std=c17 Tarea1SistemasOperativos.c -o planificador
```

La rúbrica menciona `-lpthread`; no se necesita porque este programa no usa
threads ni ninguna API de hilos. Si se quiere compilar exactamente con esa
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

## 1.1 Parseo

Se usa `getline()` para no depender de un buffer de tamaño fijo. Después se
usa `strtok_r()` para separar los campos por `:` y otro `strtok_r()` para las
dependencias.

Los IDs se guardan como strings porque la tarea permite IDs alfanuméricos.
`trim()` elimina los espacios que vienen alrededor de los campos.

Cuando el tiempo está vacío, se usa:

```c
100 + rand() % 4901
```

para obtener un tiempo entre 100 y 5000 ms.

## 1.2 DAG

Cada actividad guarda sus dependencias en `dependencias`. Además se arma
`dlist`, que es la lista inversa: si una actividad A es dependencia de B,
A guarda un puntero hacia B.

Para encontrar los IDs se usa `strcmp()` recorriendo las actividades. Esto es
O(n²), pero permite trabajar con IDs alfanuméricos arbitrarios sin agregar una
estructura más complicada.

Una dependencia que no existe en el archivo se considera un plan mal formado
y el programa termina con error.

También se revisan ciclos. Si no hay ninguna raíz al principio, se informa el
posible ciclo. Si sí hay raíces pero después quedan actividades sin resolver,
se informa como ciclo parcial.

## 2.1 Procesos y límite K

Cada actividad que queda lista se ejecuta en un proceso hijo creado con
`fork()`.

La cola `q` guarda las actividades que ya pueden comenzar. El padre mantiene
`activos` y no crea más hijos cuando llega a `K`.

Cuando se llega al límite se usa `wait()` para esperar a que termine algún
hijo. No se hace busy-waiting.

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

El hijo vuelve a sembrar `rand()` usando `time(NULL) ^ getpid()` porque
`fork()` copia el estado del generador aleatorio. Sin esto, varios hijos
creados casi al mismo tiempo podrían terminar usando la misma secuencia.

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

## Memoria

Al terminar normalmente se libera la estructura principal, los strings, las
dependencias y las listas inversas. También se liberan en los caminos de
error relevantes.

## Importante

No hay `pthread_create`, `pthread_join`, mutex, condition variables,
semaphores de hilos ni ninguna otra herramienta de threads. La concurrencia
del trabajo se implementa exclusivamente con procesos.
