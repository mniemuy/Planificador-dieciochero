#include <stdio.h>
#include <unistd.h>
#include <string.h>


struct datoken{

}


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


while(fgets(buffer, sizeof(buffer), f) != NULL){ //mientras haya datos, es cmo un read
char* sep = strtok_r(buffer, ":", &save);  //los archivos están separados por un :

while(sep != NULL){
    printf("Separación: %s\n", sep);
    sep = strtok_r(NULL, ":", &save);    
    
    }

}
//para parsear quiero meter distintos tokens para ocpar el de las dependencias como uno solo





fclose(f);
return 0;
}