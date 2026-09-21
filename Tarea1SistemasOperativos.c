#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <time.h> //para dar duracion aleatoria a las tareas q no tienen chat


struct datoken{
char* ID_Actividad;
char* Nombre_actividad;
int tiempo_ms;
char** dependencias; //la tarea dice alfanumérico, cubriendo ese caso queda así
int nd;                 //cuántas dependencias tiene
struct datoken** dlist; //lista de dependientes, puntero a tipo datoken accede a cada una d las cositas
int dcounter;  //cuenta los dependientes actuales (trabajando)
// MERGE: dlist/dcounter son de la rama de Lukas. En tu rama el struct ya no los traía,
// pero los necesitamos sí o sí para poder armar el DAG y avisarle a los dependientes.
};


// MERGE: trim() es de tu rama (Lukas no la tenía). Hace falta porque el enunciado trae
// ejemplos con espacios alrededor de ":" (ej "1 : prender_carbon : 500 :"); sin recortar,
// el ID_Actividad quedaría con espacios y el strcmp() del DAG (más abajo) no matchearía.
char* trim(char* s){ //hay que contemplar sacar los espacios en blanco q puedan haber
    while(*s == ' ')//avanza mientras lo que lee en s es "vacio"
        s++;
    if(*s == '\0')//si llegamos al final a puro saltito sin nada, entonces devolvemos el string vacío nomas
        return s;
    char* fin = s + strlen(s) - 1; //buscamos el ultimo caracter del string
    while(fin > s && *fin == ' ')//lo mismo de antes, mientras el ultimo caracter sea espacio, entonces volvemos
        fin--;
    *(fin + 1) = '\0';    //le ponemos fin justo despues del ultimo char que no era espacio
    return s;//devolvemos el string ya recortado q sigue apuntando dentro del mismo espacio de memoria, no es un nuevo string!!! ojo al charqui
}


int main(int argc, char* argv[]){
if(argc < 3 || argc > 3){
    return -1; //esto es x si algún chistosito no le da suficientes argumentos o le da de más jeje xdxd lol papu
}

srand(time(NULL)); //esto le da el verdadero corte aleatorio, ya que siempre serà distinto -- MERGE: de tu rama, Lukas no llamaba srand()

int conlimit = atoi(argv[2]); //límite de concurrencia, llega en arg cmo string, atoi lo convierte a int igual q abajo
(void)conlimit; // MERGE: todavía no se usa (falta 2.1 control de concurrencia); esto es solo para que -Wextra no tire warning de "unused variable" mientras tanto

FILE *f = fopen(argv[1], "r");// siempre pasa archivo.txt en pos 1, pos 0 ./hola - pos1 archivo.txt - pos2 K = 'num'
if(f == NULL){
    return -1;//si no se abrió
}
//strtok normal es muy poco manejable, con la versión _r nosotros controlamos el stack y dónde queda para el parseo
//en strtok si intentas tokenizar dos cosas distintas a la vez no funciona, cmo q se pisan

char *buffer = NULL; //de acá van a salir los datos q se leen en el .txt para separarlos
size_t capacidad = 0; // ahora lo haré con getline pq hace malloc internamente
char* save = NULL; //save point

int ind = 1; //pal ciclo de movimiento!! (tu versión corta decía "pal ciclo!!", queda documentada acá también)
int c = 0; // pal pre ciclo ciclo

//2
// MERGE: se eligió el conteo de líneas + getline de la rama de Lukas en vez del fgets con
// buffer[676] fijo de tu rama: con hasta 10000 actividades, una línea larga (muchas
// dependencias) se podía pasar de 676 bytes y fgets la truncaba en silencio.
while(getline(&buffer, &capacidad, f) != -1){
c++;
}
rewind(f); //conté las lineas para guardar todo con un malloc todopoderoso, esto es pa leer el archivo de nuevo

struct datoken* espacio = malloc(sizeof(struct datoken)*c); // la cantidad de lineas por la cantidad de bytes que ocupa cada cosa en el struct, no el texto de cada cosa
// MERGE: este arreglo persistente "espacio" es de la rama de Lukas. En tu rama "pama" era
// local a cada vuelta del while y se perdía apenas terminaba la iteración (solo quedaban
// los printf de debug, no había dónde consultarlo después para armar el DAG ni crear procesos).

int c2 = 0;

while(getline(&buffer, &capacidad, f) != -1){ //mientras haya datos, es cmo un read

    char* sep = strtok_r(buffer, ":", &save); //los archivos están separados por un :
    struct datoken pama;//para ir cambiando c/u

    pama.dependencias = NULL;
    pama.nd = 0; //se garantiza pase lo que pase, no depende del ind==4, ya que si no hay dependencias, no se entra al if y queda en 0, lo cual es correcto
    espacio[c2].dlist = NULL; //dag
    espacio[c2].dcounter = 0;

    while(sep != NULL){ //movimiento

        if(ind == 1){
            char* sep_limpio = trim(sep); //sin esta vaina mi loco cuando comparemos strings se nos va todo a la b por los espacios, entonces limpiamos con la super funcion nueva de antes
            pama.ID_Actividad = malloc(strlen(sep_limpio)+ 1);
            strcpy(pama.ID_Actividad, sep_limpio);
            espacio[c2].ID_Actividad = pama.ID_Actividad;
            printf("PAMAPRINT: %s\n", pama.ID_Actividad);
            ind++;
        } else if(ind == 2){
            char* sep_limpio = trim(sep); //misma vola pero este es mas superficial, es el nombre de la actividad nomas, por eso paso de sep a sep_limpio
            pama.Nombre_actividad = malloc(strlen(sep_limpio)+ 1);
            strcpy(pama.Nombre_actividad, sep_limpio);
            espacio[c2].Nombre_actividad = pama.Nombre_actividad;
            printf("PAMAPRINT: %s\n", pama.Nombre_actividad);
            ind++;
        } else if(ind == 3){
            pama.tiempo_ms = atoi(sep);
            if(pama.tiempo_ms <= 0){
                // MERGE: en tu versión original faltaba el ";" acá al final, no compilaba.
                pama.tiempo_ms = 100 + rand() % 4901; //(5000-100+1)
            }
            espacio[c2].tiempo_ms = pama.tiempo_ms;
            printf("PAMAPRINT: %d\n", pama.tiempo_ms);
            ind++;
        } else if(ind == 4){//casi siempre se entra aca pero el problema es que si no hay dependencias, no se entra y queda en 0, lo cual es correctoes que no sea siempre...
            pama.dependencias = NULL;
            pama.nd = 0;
            char* save2 = NULL;

            char* aux = strtok_r(sep, " ,\n", &save2); // el espacio está a propósito

            while(aux != NULL){
                pama.dependencias = realloc(pama.dependencias, (pama.nd + 1)*sizeof(char*));
                pama.dependencias[pama.nd] = malloc(strlen(aux) + 1);
                strcpy(pama.dependencias[pama.nd], aux);
                pama.nd++;
                aux = strtok_r(NULL, " ,\n", &save2); // el espacio está a propósito
            }

            printf("Actividad %s tiene %d dependencias: ", pama.ID_Actividad, pama.nd);
            for (int i = 0; i < pama.nd; i++) {
                printf("[%s] ", pama.dependencias[i]);
            }
            printf("\n");
            //verificar la verificación (estoy chato basta por favor)

            espacio[c2].dependencias = pama.dependencias;//UN PUNTERO
            espacio[c2].nd = pama.nd;
        }

        //lo dejé pa hoy efectivamente
        printf("Separación: %s\n", sep);

        sep = strtok_r(NULL, ":", &save);
    }
    c2++;
    ind = 1;
}
//para parsear quiero meter distintos tokens para ocpar el de las dependencias como uno solo

for(int i = 0; i < c; i++){//para todas las lineas parseadas
    for(int j = 0; j < espacio[i].nd; j++){ //para todas las dependencias de cada linea
        int encontrada = 0; // MERGE: bandera nueva para poder avisar si un ID de dependencia no existe en el plan (antes fallaba en silencio)
        for(int k = 0; k < c; k++){
         if(strcmp(espacio[k].ID_Actividad, espacio[i].dependencias[j]) == 0){// si son iguales, realloc
                espacio[k].dlist = realloc(espacio[k].dlist, (espacio[k].dcounter + 1) * sizeof(struct datoken*));
                // MERGE: esto faltaba en tu rama (Lukas): el realloc solo agrandaba dlist
                // pero nunca guardaba el puntero a la actividad "i" ni avanzaba dcounter,
                // entonces dlist quedaba con memoria basura y dcounter siempre en 0
                // (el DAG nunca quedaba realmente armado). Se agregan estas dos líneas:
                espacio[k].dlist[espacio[k].dcounter] = &espacio[i];
                espacio[k].dcounter++;
                encontrada = 1;
                break;
         }
        }
        if(!encontrada){
            fprintf(stderr, "Advertencia: la actividad %s depende de '%s', que no existe en el plan.\n",
                    espacio[i].ID_Actividad, espacio[i].dependencias[j]);
        }
    }
}// entro a actividades, luego a dependencias, y por cada dependencia reviso actividades nuevamente, para poder juntarlas
//según la tarea son alfanuméricas, es decisión de diseño un poco más ineficiente, pero funciona para cualquier nombre de id que le pongan
//strcmp va comparando una a una actividades k, con dependencias j de una actividad i
//si son iguales, hace que en dlist del espacio en la posición k, se haga un espacio nuevo de memoria, manteniendo el anterior para tener la lista completa de dependientes
//el primer argumento es donde, y el segundo es la cantidad de dependientes que hay +1 por los espacios de cada item en un struct datoken
//osea, por ejemplo 1, 4 es dependiente de este. entonces 1 hace un puntero a 4, ya que 1 tiene que ver quien lo está esperando para "avisar"


for (int i = 0; i < c; i++) {
    printf("Actividad %s tiene %d dependientes: ", espacio[i].ID_Actividad, espacio[i].dcounter);
    for (int x = 0; x < espacio[i].dcounter; x++) {
        printf("[%s] ", espacio[i].dlist[x]->ID_Actividad);
    }
    printf("\n");
}

//verificar la verificación nuevamente
free(buffer);//o morimos

fclose(f);//terminemos esto como lo empezamos luciano, juntos
return 0;
}