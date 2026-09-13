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
};


int main(int argc, char* argv[]){
if(argc < 3 || argc > 3){
    return -1; //esto es x si algún chistosito no le da suficientes argumentos o le da de más jeje
}
FILE *f = fopen(argv[1], "r");// siempre pasa archivo.txt en pos 1, pos 0 ./hola - pos1 archivo.txt - pos2 K = 'num'
if(f == NULL){
    return -1;//si no se abrió
}
//strtok normal es muy poco manejable, con la versión _r nosotros controlamos el stack y dónde queda para el parseo
//en strtok si intentas tokenizar dos cosas distintas a la vez no funciona, cmo q se pisan

char buffer[676]; //de acá van a salir los datos q se leen en el .txt para separarlos
char* save = NULL; //save point

int ind = 1; //pal ciclo!!

    

while(fgets(buffer, sizeof(buffer), f) != NULL){ //mientras haya datos, es cmo un read

    char* sep = strtok_r(buffer, ":", &save);  //los archivos están separados por un :
    struct datoken pama;//para ir cambiando c/u

    while(sep != NULL){        
    
     if(ind == 1){
         pama.ID_Actividad = malloc(strlen(sep)+ 1);
        strcpy(pama.ID_Actividad, sep);
        printf("PAMAPRINT: %s\n", pama.ID_Actividad);
        ind++;
    } else if(ind == 2){
        pama.Nombre_actividad = malloc(strlen(sep)+ 1);
        strcpy(pama.Nombre_actividad, sep);
        printf("PAMAPRINT: %s\n", pama.Nombre_actividad);
        ind++; 
    } else if(ind == 3){
        pama.tiempo_ms = atoi(sep);
    printf("PAMAPRINT: %d\n", pama.tiempo_ms);
        ind++;
    }

    //tengo q hacer una para el 4 pero tengo q manejar el arreglo de punteros a punteros y lo dejaré pa mañana
    printf("Separación: %s\n", sep);

    sep = strtok_r(NULL, ":", &save);    
  
    }
  ind = 1;    
}   
//para parsear quiero meter distintos tokens para ocpar el de las dependencias como uno solo





fclose(f);
return 0;
}