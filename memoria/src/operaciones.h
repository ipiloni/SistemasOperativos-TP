#ifndef OPERACIONES_H
#define OPERACIONES_H

#include "main.h"
#include "estructuras.h"

void pedido_crear_proceso(t_buffer* buffer, int socket_cliente);
void pedido_eliminar_proceso(t_buffer* buffer, int socket_cliente);
void pedido_crear_segmento(t_buffer * buffer, int socket_cliente);
void pedido_eliminar_segmento(t_buffer * buffer, int socket_cliente);
void pedido_compactar(int socket_cliente);
void pedido_leer_espacio_usuario(t_buffer* buffer, int socket_cliente, uint8_t idCliente);
void pedido_escribir_espacio_usuario(t_buffer* buffer, int socket_cliente, uint8_t idCliente);
void serializarTablaSegmentos(t_buffer* buffer,t_list * tablaSegmentos);
void serializarString(t_buffer* buffer,char* leido);
#endif /* OPERACIONES_H */
