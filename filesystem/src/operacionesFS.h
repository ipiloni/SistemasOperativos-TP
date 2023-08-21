#ifndef OPERACIONESFS_H_
#define OPERACIONESFS_H_

#include "main.h"

void crear_archivo(char* nombreArchivo, int socket_kernel);
void abrir_archivo(char* nombreArchivo, int socket_kernel);
void truncar_archivo(char* nombreArchivo, int tamanio);
void acceder_archivo(char* nombreArchivo, int punteroArchivo, int dir_fisica ,int tamanioAAcceder, int pid ,op_code modo, int socket_memoria);
void accederADirecto(int offsetDentroDeBloque , int tamanioAcceso, int punteroDirecto,char** string, op_code modo,char *nombreArchivo,int bloqueDelArchivo);
void accederAIndirectosRestantes(int bloqueIndirectoInicial,int tamanioRestanteParaIndirectos, int punteroIndirecto,char** stream, op_code modo,char* nombreArchivo);
char * recibir_nombre_de_kernel(t_buffer* buffer);
int buscarBloqueLibre();
void levantar_bitmap();
void enviar_mensaje_a_kernel(int socket_kernel, op_code codigo_operacion);
void imprimirFCBs(t_list* lista);

























#endif /* OPERACIONESFS_H_ */
