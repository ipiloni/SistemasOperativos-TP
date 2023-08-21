#ifndef SRC_COMUNICACION_H_
#define SRC_COMUNICACION_H_

#include "main.h"

//Kernel
int comunicarse_a_kernel(int, int);
int esperar_kernel(int);
t_contexto* recibir_contexto(int, t_buffer*);
void devolver_contexto_con_dir_fisica(int, cod_instruccion, uint32_t);
void devolver_contexto(int, cod_instruccion);
void enviar_direccion_fisica(int socket, uint32_t direccion_fisica);
void serializar_contexto_parcial(t_buffer*, cod_instruccion);
void eliminar_contexto(t_contexto* contexto);
//Conexion a memoria
int conectarse_a_memoria();

#endif /* SRC_COMUNICACION_H_ */
