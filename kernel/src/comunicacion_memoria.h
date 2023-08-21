#ifndef SRC_COMUNICACION_MEMORIA_H_
#define SRC_COMUNICACION_MEMORIA_H_

#include "main.h"

void* comunicacion_memoria(void* arg);
t_motivo_desalojo crear_segmento(t_instruccion* instruccion_pendiente, int socket_memoria);
void eliminar_segmento(int id_segmento, int socket_memoria);
void* procesar_eliminacion_memoria(void *arg);

#endif /* SRC_COMUNICACION_MEMORIA_H_ */
