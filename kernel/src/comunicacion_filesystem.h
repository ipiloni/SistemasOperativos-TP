#ifndef SRC_COMUNICACION_FILESYSTEM_H_
#define SRC_COMUNICACION_FILESYSTEM_H_

#include "main.h"

typedef struct {
    char* nombre_archivo;
    uint32_t puntero;
} t_archivo_proceso;

typedef struct {
    char* nombre_archivo;
    t_queue* cola_esperando;
    bool bit_de_uso;
} t_archivo_global;

void* comunicacion_FS(void* arg);
t_archivo_global *buscarArchivoPorNombreGlobal(char * nombre_archivo);
t_archivo_proceso *buscarArchivoPorNombreLocal(char * nombre_archivo, t_list * listaLocal);
void eliminar_archivo_global(t_archivo_global* archivo);
void eliminar_archivo_local(t_archivo_proceso* archivo);
t_archivo_global* inicializar_archivo_global(char* nombre_archivo);
t_archivo_proceso* inicializar_archivo_local(char* nombre_archivo);
bool estaEnUso(t_archivo_global* archivo);
bool estaEnLaTablaGlobal(t_archivo_global* archivo);
void imprimir_archivos_abiertos();
void iterador_print_archivo_global(t_archivo_global* archivo_global);
#endif /* SRC_COMUNICACION_FILESYSTEM_H_ */
