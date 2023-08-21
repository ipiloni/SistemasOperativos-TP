#ifndef SRC_CONEXIONES_H_
#define SRC_CONEXIONES_H_

#include "main.h"

void* comunicacion(void* argumentos);
int responder_handshake(int socket_cliente);
void procesar_conexion(int socket_cliente,handshake idCliente);
#endif /* SRC_CONEXIONES_H_ */
