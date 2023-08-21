#include "conexiones.h"

//Conexionado
void* comunicacion(void* argumentos) {
	int socket_cliente;
	socket_cliente = *((int *) argumentos);
	free(argumentos);

	pthread_detach(pthread_self());

	handshake idCliente = responder_handshake(socket_cliente);

	if (idCliente == -1) {
		log_info(logger, "Fallo el handshake. Aborto Hilo O\n");
		return NULL; //Esta linea suicida al hilo
	}

	procesar_conexion(socket_cliente,idCliente);
	return 0;
}


int responder_handshake(int socket_cliente) {
	uint32_t handshake;
	uint32_t resultOk = 0;
	uint32_t resultError = -1;

	recv(socket_cliente, &handshake, sizeof(uint32_t), MSG_WAITALL);

	if(handshake > 4 || handshake < 0){
		log_error(logger,"Se trato de conectar un cliente sin sentido");
		send(socket_cliente, &resultError, sizeof(uint32_t), 0);
		return -1;
	}

	log_info(logger, "Bienvenido %s :)",obtener_modulo(handshake));
	send(socket_cliente, &resultOk, sizeof(uint32_t), 0);
	handshakesCorrectos++;
	sem_post(&handshakeTerminado);

	return handshake;

}

void procesar_conexion(int socket_cliente,handshake idCliente) {
	sem_wait(&semaforoProcesarConexion[idCliente-1]);

	while (1){// Nos quedamos en while recibiendo paquetes de la conexion

		t_paquete* paquete = crear_paquete();

		printf("\n");

		paquete = recibir_paquete(paquete, socket_cliente);

		if (paquete->codigo_operacion == ERROR) break;


		if(idCliente == KERNEL) {
			switch (paquete->codigo_operacion) {
			case CREAR_PROCESO:
				pedido_crear_proceso(paquete->buffer, socket_cliente);
				break;
			case ELIMINAR_PROCESO:
				pedido_eliminar_proceso(paquete->buffer, socket_cliente);
				break;
			case CREAR_SEGMENTO:
				pedido_crear_segmento(paquete->buffer, socket_cliente);
				break;
			case ELIMINAR_SEGMENTO:
				pedido_eliminar_segmento(paquete->buffer, socket_cliente);
				break;
			default:
				log_warning(logger,"Operacion desconocida. No quieras meter la pata");
				break;
			}
		} else if (idCliente == FILESYSTEM || idCliente == CPU) {
			switch (paquete->codigo_operacion) {
				case ESCRIBIR_ESPACIO_USUARIO:
				pedido_escribir_espacio_usuario(paquete->buffer, socket_cliente, idCliente);
				break;
				case LEER_ESPACIO_USUARIO:
				pedido_leer_espacio_usuario(paquete->buffer, socket_cliente, idCliente);
				break;
				default:
				log_warning(logger,"Operacion desconocida. No quieras meter la pata");
				break;
			}
		} else {
			log_warning(logger,"Cliente desconocido");
		}

	eliminar_paquete(paquete);

	}

	log_info(logger, "Se ha desconectado el cliente con id: %d ", idCliente);
}
