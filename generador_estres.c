#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

//este programa aparte genera un plan.txt gigante para la prueba de estrés
//(criterio 2.4). cada actividad depende de 0 a 3 actividades ANTERIORES
//elegidas al azar (nunca de una posterior), asi el grafo queda
//garantizado aciclico sin tener que revisar nada despues
int main(int argc, char* argv[]){
if(argc != 2){
printf("uso: %s cantidad_actividades\n", argv[0]);
return -1;
    }

int n = atoi(argv[1]);
if(n <= 0){
printf("la cantidad de actividades debe ser mayor que 0.\n");
return -1;
}

srand(time(NULL));

FILE* f = fopen("plan_estres.txt", "w");
if(f == NULL){
perror("no se pudo crear el archivo");
return -1;
    }

for(int i = 1; i <= n; i++){
fprintf(f, "%d : actividad_%d : :", i, i); //tiempo vacio a proposito, que lo sortee el parser

if(i > 1){
int cant_deps = rand() % 4; //0 a 3 dependencias
for(int j = 0; j < cant_deps; j++){
int dep = 1 + rand() % (i - 1); //siempre un ID menor a i, nunca ciclos
fprintf(f, " %d", dep);
if(j < cant_deps - 1) fprintf(f, ",");
            }
        }
fprintf(f, "\n");
    }

fclose(f);
printf("plan_estres.txt generado con %d actividades.\n", n);
return 0;
}
