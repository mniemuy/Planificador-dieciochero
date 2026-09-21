#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>


struct datoken{
char* ID_Actividad;
char* Nombre_actividad;
int tiempo_ms;
char** dependencias; //la tarea dice alfanumérico, cubriendo ese caso queda así
int nd;                 //cuántas dependencias tiene
struct datoken** dlist; //lista de dependientes, puntero a tipo datoken accede a cada una d las cositas
int dcounter;  //cuenta los dependientes actuales (trabajando)
};


int main(int argc, char* argv[]){

if(argc < 3 || argc > 3){
    return -1; //esto es x si algún chistosito no le da suficientes argumentos o le da de más jeje
}

int conlimit = atoi(argv[2]); //límite de concurrencia, llega en arg cmo string, atoi lo convierte a int igual q abajo


FILE *f = fopen(argv[1], "r");// siempre pasa archivo.txt en pos 1, pos 0 ./hola - pos1 archivo.txt - pos2 K = 'num'
if(f == NULL){
    return -1;//si no se abrió
}
//strtok normal es muy poco manejable, con la versión _r nosotros controlamos el stack y dónde queda para el parseo
//en strtok si intentas tokenizar dos cosas distintas a la vez no funciona, cmo q se pisan

char *buffer = NULL;
size_t capacidad = 0; // ahora lo haré con getline pq hace malloc internamente
char* save = NULL; //save point

int ind = 1; //pal ciclo de movimiento!!
int c = 0; // pal pre ciclo ciclo
//2
while(getline(&buffer, &capacidad, f) != -1){
c++;
}   

rewind(f); //conté las lineas para guardar todo con un malloc todopoderoso, esto es pa leer el archivo de nuevo

struct datoken* espacio = malloc(sizeof(struct datoken)*c); // la cantidad de lineas por la cantidad de bytes que ocupa cada cosa en el struct, no el texto de cada cosa


int c2 = 0;

while(getline(&buffer, &capacidad, f) != -1){ //mientras haya datos, es cmo un read

    char* sep = strtok_r(buffer, ":", &save);  //los archivos están separados por un :
    struct datoken pama;//para ir cambiando c/u
 
    espacio[c2].dlist = NULL; //dag
    espacio[c2].dcounter = 0;

    while(sep != NULL){        //movimiento
    
     if(ind == 1){
        pama.ID_Actividad = malloc(strlen(sep)+ 1);
        strcpy(pama.ID_Actividad, sep);
         espacio[c2].ID_Actividad = pama.ID_Actividad;
        ind++;
    } else if(ind == 2){
        
        pama.Nombre_actividad = malloc(strlen(sep)+ 1);
        strcpy(pama.Nombre_actividad, sep);
        espacio[c2].Nombre_actividad = pama.Nombre_actividad;
        ind++; 
    } else if(ind == 3){
        pama.tiempo_ms = atoi(sep);
         espacio[c2].tiempo_ms = pama.tiempo_ms;

        ind++;
    } else if(ind == 4){
        pama.dependencias = NULL;
        pama.nd = 0;
        char* save2 = NULL;

        char* aux = strtok_r(sep, " ,\n", &save2); // el espacio está a propósito
        
        while(aux != NULL){
            pama.dependencias = realloc(pama.dependencias, (pama.nd + 1)*sizeof(char*)); // nos importa el contexto, lo q había antes

            pama.dependencias[pama.nd] = malloc(strlen(aux) + 1);
            strcpy(pama.dependencias[pama.nd], aux);
            
            pama.nd++;
            aux = strtok_r(NULL, " ,\n", &save2); // el espacio está a propósito
        }
        espacio[c2].dependencias = pama.dependencias;//UN PUNTERO
        espacio[c2].nd = pama.nd;
        
        
    } 


   
    sep = strtok_r(NULL, ":", &save);    
  
    }
    c2++;
    ind = 1;    
}   
//para parsear quiero meter distintos tokens para ocpar el de las dependencias como uno solo

for(int i = 0; i < c; i++){//para todas las lineas parseadas    
    for(int j = 0; j < espacio[i].nd; j++){ //para todas las dependencias de cada linea
        for(int k = 0; k < c; k++){
         if(strcmp(espacio[k].ID_Actividad, espacio[i].dependencias[j]) == 0){// si son iguales, realloc
                espacio[k].dlist = realloc(espacio[k].dlist, (espacio[k].dcounter + 1) * sizeof(struct datoken*)); 

                espacio[k].dlist[espacio[k].dcounter] = &espacio[i];
                espacio[k].dcounter++;
                break;
         }
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