#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <time.h> //para dar duracion aleatoria a las tareas q no tienen chat
#include <signal.h> //para la inspeccion de la seremi (Ctrl+C)
#include <errno.h> //para saber si wait() se corto por una señal

#define TAM_MSG 128 //tamaño del mensaje acotado que viaja por cada pipe

struct datoken{
char* ID_Actividad;
char* Nombre_actividad;
int tiempo_ms;
char** dependencias; //la tarea dice alfanumérico, cubriendo ese caso queda así
int nd;                 //cuántas dependencias tiene
struct datoken** dlist; //lista de dependientes, puntero a tipo datoken accede a cada una d las cositas
int dcounter;  //cuenta los dependientes actuales (trabajando)
int restantes; //para ir restando en otra cosa que no sea pama.nd
pid_t pid;
int fail; // 1 si la actividad murió, los dependientes se abortan(jaj)
int corriendo; // 1 mientras el proceso de esta actividad sigue vivo, para el Ctrl+C
int tuberia[2]; //el pipe de esta actividad. tuberia[0]=lectura (la usa el padre), tuberia[1]=escritura (la usa el hijo)
char insumo[TAM_MSG]; //el mensaje que la actividad manda por la tuberia cuando termina
};

char* trim(char* s){ //hay que contemplar sacar los espacios en blanco q puedan haber
    while(*s == ' ')
        s++;
    if(*s == '\0')
        return s;
    char* fin = s + strlen(s) - 1;
    while(fin > s && *fin == ' ')
        fin--;
    *(fin + 1) = '\0';
    return s;//devolvemos el string ya recortado q sigue apuntando dentro del mismo espacio de memoria, no es un nuevo string!!! ojo al charqui
}//XD

volatile sig_atomic_t llego_la_seremi = 0;

void manejador_sigint(int sig){
    (void)sig; //no lo usamos, pero hay que declararlo igual, si no -Wextra reclama
    llego_la_seremi = 1; //nada mas aca! nada de printf ni malloc en un manejador
}

void liberar_espacio(struct datoken* espacio, int c){
    if(espacio == NULL)
        return;

    for(int i = 0; i < c; i++){
        free(espacio[i].ID_Actividad);
        free(espacio[i].Nombre_actividad);

        for(int j = 0; j < espacio[i].nd; j++)
            free(espacio[i].dependencias[j]);

        free(espacio[i].dependencias);
        free(espacio[i].dlist);
    }

    free(espacio);
}

void abortar_dependientes(struct datoken* espacio, int idx, int* q, int* qend){
    for(int x = 0; x < espacio[idx].dcounter; x++){
        struct datoken* dep = espacio[idx].dlist[x];
        dep->restantes--;
        dep->fail = 1;

        if(dep->restantes == 0){
            q[(*qend)++] = (int)(dep - espacio);
        }
    }
}

int main(int argc, char* argv[]){
if(argc < 3 || argc > 3){
    return -1; //esto es x si algún chistosito no le da suficientes argumentos o le da de más jeje xdxd lol papu
}

int conlimit = atoi(argv[2]); //límite de concurrencia, llega en arg cmo string, atoi lo convierte a int igual q abajo
if(conlimit <= 0){
    fprintf(stderr, "Error: K debe ser mayor que 0.\n");
    return -1;
}

srand(time(NULL)); //esto le da el verdadero corte aleatorio, ya que siempre serà distinto

struct sigaction sa;
sa.sa_handler = manejador_sigint;
sigemptyset(&sa.sa_mask);
sa.sa_flags = 0; //A PROPOSITO sin SA_RESTART: asi wait() se corta con EINTR
                 //apenas llega la señal, en vez de seguir esperando a que
                 //termine el hijo actual antes de enterarse
sigaction(SIGINT, &sa, NULL);

FILE *f = fopen(argv[1], "r");// siempre pasa archivo.txt en pos 1, pos 0 ./hola - pos1 archivo.txt - pos2 K = 'num'
if(f == NULL){
    perror("no se pudo abrir el archivo");
    return -1;//si no se abrió
}

//strtok normal es muy poco manejable, con la versión _r nosotros controlamos el stack y dónde queda para el parseo
//en strtok si intentas tokenizar dos cosas distintas a la vez no funciona, cmo q se pisan

char *buffer = NULL; //de acá van a salir los datos q se leen en el .txt para separarlos
size_t capacidad = 0; // con getline, que hace malloc internamente y agranda solo si una linea es mas larga
char* save = NULL; //save point

int ind = 1; //pal ciclo de movimiento!!
int c = 0; // pal pre ciclo ciclo

while(getline(&buffer, &capacidad, f) != -1){
c++;
}

rewind(f); //conté las lineas para guardar todo con un malloc todopoderoso, esto es pa leer el archivo de nuevo

struct datoken* espacio = malloc(sizeof(struct datoken)*c);
if(c > 0 && espacio == NULL){
    perror("no se pudo reservar memoria");
    free(buffer);
    fclose(f);
    return -1;
}

int c2 = 0;
int malformado = 0;

while(getline(&buffer, &capacidad, f) != -1){
    char* sep = strtok_r(buffer, ":", &save);
    struct datoken pama;

    pama.ID_Actividad = NULL;
    pama.Nombre_actividad = NULL;
    pama.dependencias = NULL;
    pama.nd = 0;
    pama.tiempo_ms = 0;

    espacio[c2].dlist = NULL;
    espacio[c2].dcounter = 0;
    espacio[c2].fail = 0;
    espacio[c2].corriendo = 0;
    espacio[c2].restantes = 0;
    espacio[c2].pid = -1;

    while(sep != NULL){

        if(ind == 1){
            char* sep_limpio = trim(sep);
            pama.ID_Actividad = malloc(strlen(sep_limpio)+ 1);
            if(pama.ID_Actividad == NULL){
                malformado = 1;
                break;
            }
            strcpy(pama.ID_Actividad, sep_limpio);
            espacio[c2].ID_Actividad = pama.ID_Actividad;
            ind++;
        } else if(ind == 2){
            char* sep_limpio = trim(sep);
            pama.Nombre_actividad = malloc(strlen(sep_limpio)+ 1);
            if(pama.Nombre_actividad == NULL){
                malformado = 1;
                break;
            }
            strcpy(pama.Nombre_actividad, sep_limpio);
            espacio[c2].Nombre_actividad = pama.Nombre_actividad;
            ind++;
        } else if(ind == 3){
            pama.tiempo_ms = atoi(sep);
            ind++;
        } else if(ind == 4){
            char* save2 = NULL;
            char* aux = strtok_r(sep, " ,\n", &save2);

            while(aux != NULL){
                char** nuevas = realloc(pama.dependencias, (pama.nd + 1)*sizeof(char*));
                if(nuevas == NULL){
                    malformado = 1;
                    break;
                }

                pama.dependencias = nuevas;
                pama.dependencias[pama.nd] = malloc(strlen(aux) + 1);
                if(pama.dependencias[pama.nd] == NULL){
                    malformado = 1;
                    break;
                }

                strcpy(pama.dependencias[pama.nd], aux);
                pama.nd++;
                aux = strtok_r(NULL, " ,\n", &save2);
            }

            if(malformado)
                break;
        }

        sep = strtok_r(NULL, ":", &save);
    }

    //estas 4 lineas quedan FUERA del while de arriba un purpleXD: asi se
    //ejecutan siempre, tenga la linea 4 campos completos o no (nodo raiz
    //sin dependencias, o el caso extremo de la ultima linea sin salto de
    //linea final que ni al ind==3 llega)
    if(pama.tiempo_ms <= 0){
        pama.tiempo_ms = 100 + rand() % 4901; //(5000-100+1)
    }

    espacio[c2].tiempo_ms = pama.tiempo_ms;
    espacio[c2].dependencias = pama.dependencias;
    espacio[c2].nd = pama.nd;
    espacio[c2].restantes = pama.nd;

    if(pama.ID_Actividad == NULL || pama.Nombre_actividad == NULL)
        malformado = 1;

    c2++;
    ind = 1;
}

if(malformado || c2 != c){
    fprintf(stderr, "Error: el plan esta mal formado o no se pudo reservar memoria.\n");
    liberar_espacio(espacio, c2);
    free(buffer);
    fclose(f);
    return -1;
}

//armamos el DAG de verdad. por cada actividad, por cada dependencia suya,
//buscamos que actividad tiene ese ID y le avisamos "soy el alumno en practica"
for(int i = 0; i < c; i++){
    for(int j = 0; j < espacio[i].nd; j++){
        int encontrada = 0;
        for(int k = 0; k < c; k++){
            if(strcmp(espacio[k].ID_Actividad, espacio[i].dependencias[j]) == 0){
                espacio[k].dlist = realloc(espacio[k].dlist, (espacio[k].dcounter + 1) * sizeof(struct datoken*));
                if(espacio[k].dlist == NULL){
                    fprintf(stderr, "Error: no se pudo armar el DAG.\n");
                    liberar_espacio(espacio, c);
                    free(buffer);
                    fclose(f);
                    return -1;
                }

                espacio[k].dlist[espacio[k].dcounter] = &espacio[i];
                espacio[k].dcounter++;
                encontrada = 1;
                break;
            }
        }

        if(!encontrada){
            fprintf(stderr, "Error: la actividad %s depende de '%s', que no existe en el plan.\n",
                    espacio[i].ID_Actividad, espacio[i].dependencias[j]);
            malformado = 1;
        }
    }
}

if(malformado){
    liberar_espacio(espacio, c);
    free(buffer);
    fclose(f);
    return -1;
}

//según la tarea son alfanuméricas, es decisión de diseño un poco más leseada,
//pero funciona para cualquier nombre de id que le pongan
//strcmp va comparando una a una actividades k, con dependencias j de una actividad i
//si son iguales, hace que en dlist del espacio en la posición k, se haga un espacio
//nuevo de memoria, manteniendo el anterior para tener la lista completa

//creacion d procesos
int* q = malloc(sizeof(int) * c);
if(c > 0 && q == NULL){
    perror("no se pudo reservar memoria");
    liberar_espacio(espacio, c);
    free(buffer);
    fclose(f);
    return -1;
}

int qini = 0, qend = 0;

for(int i = 0; i < c; i++){
    if(espacio[i].restantes == 0){
        q[qend] = i;
        qend = qend + 1;
    }
}

if(qend == 0 && c > 0){
    fprintf(stderr, "Error: ninguna actividad esta lista para empezar (posible ciclo en las dependencias). Revise el plan.\n");
    free(q);
    liberar_espacio(espacio, c);
    free(buffer);
    fclose(f);
    return -1;
}

int activos = 0;
int procesadas = 0;

printf("=== arrancando la ramada, K=%d ===\n", conlimit);

while((qini < qend || activos > 0) && !llego_la_seremi){

    while(activos < conlimit && qini < qend && !llego_la_seremi){
        int idx = q[qini++];

        if(espacio[idx].fail){
            //esta actividad ya viene marcada como abortada porque alguna
            //dependencia suya fallo antes. no la ejecutamos por ende, pero
            //igual hay que avisarle a SUS subcontratados que se abortan tambien
            abortar_dependientes(espacio, idx, q, &qend);
            procesadas++;
            continue; //no gastamos cupo de K en algo que ni se ejecuta
        }

        if(pipe(espacio[idx].tuberia) == -1){
            perror("pipe");
            espacio[idx].fail = 1;
            abortar_dependientes(espacio, idx, q, &qend);
            procesadas++;
            continue;
        }

        pid_t pid = fork();

        if(pid == 0){
            close(espacio[idx].tuberia[0]);

            //ojardopolis aca: fork() copia indiscriminadamente, incluido el estado interno
            //de rand()!!!!!1. sin resembrar aca, todos los hijos heredarían la
            //MISMA secuencia del padre y el "azar" del fallo saldria
            //identico en varios al mismo tiempo. getpid() nos asegura una
            //semilla distinta por cada hijo aunque nazcan en el mismo segundo
            srand(time(NULL) ^ getpid());

            struct timespec ts;
            ts.tv_sec = espacio[idx].tiempo_ms / 1000;
            ts.tv_nsec = (espacio[idx].tiempo_ms % 1000) * 1000000L;
            nanosleep(&ts, NULL); //usleep no esta declarado en modo estricto POSIX 2008, por eso nanosleep

            int exito = (rand() % 10 != 0); //1 de cada 10 actividades falla, a proposito, para poder probar el aislamiento de errores (spoiler funciona bien)

            char msg[TAM_MSG];
            if(exito){
                snprintf(msg, TAM_MSG, "%s:listo", espacio[idx].Nombre_actividad);
            } else {
                snprintf(msg, TAM_MSG, "%s:fallo", espacio[idx].Nombre_actividad);
            }

            write(espacio[idx].tuberia[1], msg, strlen(msg) + 1);
            close(espacio[idx].tuberia[1]);

            _exit(exito ? 0 : 1);

        } else if(pid > 0){
            close(espacio[idx].tuberia[1]);
            espacio[idx].pid = pid;
            espacio[idx].corriendo = 1;
            activos++;

        } else {
            //fork() fallo (raro, pero con 10000 actividades todo es posible)
            close(espacio[idx].tuberia[0]);
            close(espacio[idx].tuberia[1]);
            espacio[idx].fail = 1;
            abortar_dependientes(espacio, idx, q, &qend);
            procesadas++;
        }
    }

    if(activos == 0){
        if(qini < qend)
            continue;
        break;
    }

    int status;
    pid_t pid_terminado = wait(&status);

    if(pid_terminado == -1){
        if(errno == EINTR){
            break; //nos interrumpio una señal (la seremi), cortamos
        }
        continue;
    }

    activos--;

    int idx_terminado = -1;
    for(int i = 0; i < c; i++){
        if(espacio[i].pid == pid_terminado){
            idx_terminado = i;
            break;
        }
    }

    if(idx_terminado == -1)
        continue;

    espacio[idx_terminado].corriendo = 0;

    //leemos el mensaje que la actividad mando por su pipe antes de morir.
    //como wait() recien confirmo que termino, el mensaje ya estaba esperando
    ssize_t leidos = read(espacio[idx_terminado].tuberia[0],
                          espacio[idx_terminado].insumo, TAM_MSG - 1);
    if(leidos > 0)
        espacio[idx_terminado].insumo[leidos] = '\0';
    else
        espacio[idx_terminado].insumo[0] = '\0';

    close(espacio[idx_terminado].tuberia[0]);

    int fallo_real = WIFEXITED(status) && WEXITSTATUS(status) != 0;
    if(fallo_real)
        espacio[idx_terminado].fail = 1;

    printf("[%s] %s\n", espacio[idx_terminado].ID_Actividad, espacio[idx_terminado].insumo);
    procesadas++;

    for(int x = 0; x < espacio[idx_terminado].dcounter; x++){
        struct datoken* dep = espacio[idx_terminado].dlist[x];
        dep->restantes--;

        if(espacio[idx_terminado].fail)
            dep->fail = 1; //se propaga el fallo hacia adelante en la cadena

        if(dep->restantes == 0)
            q[qend++] = (int)(dep - espacio);
    }
}

if(llego_la_seremi){
    fprintf(stderr, "\nVamos cerrando \n");

    for(int i = 0; i < c; i++){
        if(espacio[i].corriendo)
            kill(espacio[i].pid, SIGTERM);
    }

    for(int i = 0; i < c; i++){
        if(espacio[i].corriendo){
            waitpid(espacio[i].pid, NULL, 0);
            espacio[i].corriendo = 0;
        }
    }

    free(q);
    liberar_espacio(espacio, c);
    free(buffer);
    fclose(f);
    return 1;
}

if(procesadas < c){
    fprintf(stderr, "Error: quedaron actividades y sin resolver (posible ciclo en las dependencias)\n");
    free(q);
    liberar_espacio(espacio, c);
    free(buffer);
    fclose(f);
    return -1;
}

free(q);

liberar_espacio(espacio, c);
free(buffer);//o morimos
fclose(f);//terminemos esto como lo empezamos luciano, juntos
return 0;
}
