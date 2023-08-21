#ifndef FILESYSTEM_H_
#define FILESYSTEM_H_

#include <utils/utils.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <commons/log.h>
#include <commons/string.h>
#include <commons/config.h>
#include <commons/bitarray.h>
#include <readline/readline.h>
#include <commons/collections/list.h>
#include <pthread.h>
#include <semaphore.h>
#include <commons/collections/list.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>


typedef struct{
	char *ip;
	char *puerto_memoria;
	char *path_bitmap;

}t_conexion_info_memoria;

#include "setupFilesystem.h"

extern t_log* logger;
extern t_config* config;
extern t_bitarray* bitmap_bloques;
extern void* bitarray;
extern size_t tamanioBitArray;
extern int tamanioBloque;
extern char* pathArchBloques;





#include "operacionesFS.h"



void leer_consola(t_log*);
void paquete(int);
void terminar_programa(int, t_log*, t_config*);
void procesar_conexion(int, int);
int conectar_a_memoria(t_conexion_info_memoria* info_memoria);

#endif /* FILESYSTEM_H_ */
