#include "operaciones.h"

void pedido_crear_proceso(t_buffer* buffer, int socket_cliente){
	//Deserializo el buffer
	uint32_t pid;
	void* stream = buffer->stream;
	memcpy(&pid, stream, sizeof(uint32_t));

	//Creo el proceso
	t_list* tablaSegmentos = crearProceso(pid);

	t_paquete * paquete = crear_paquete();
	int tamanioSerializado = 0;
	void* datosSerializados = serializar_lista_segmentos(tablaSegmentos, &tamanioSerializado);

	paquete->buffer->size = tamanioSerializado;
	paquete->buffer->stream = datosSerializados;
	paquete->codigo_operacion = TABLA_SEGMENTOS;

	enviar_paquete(paquete, socket_cliente);

	eliminar_paquete(paquete);
}

void pedido_eliminar_proceso(t_buffer* buffer, int socket_cliente){
	//Deserializo el buffer
	uint32_t pid;
	void* stream = buffer->stream;
	memcpy(&pid, stream, sizeof(uint32_t));
	//Elimino el proceso
	eliminarProceso(pid);

}

void pedido_crear_segmento(t_buffer* buffer, int socket_cliente){
	//Deserializo
	uint32_t pid;
	uint32_t idSegmento;
	uint32_t tamanio_segmento;
	void* stream = buffer->stream;
	memcpy(&pid, stream, sizeof(uint32_t));
	stream+=sizeof(uint32_t);
	memcpy(&idSegmento, stream, sizeof(uint32_t));
	stream+=sizeof(uint32_t);
	memcpy(&tamanio_segmento, stream, sizeof(uint32_t));

	op_code respuesta = puedoCrearSegmento(pid, idSegmento, tamanio_segmento);

	switch (respuesta) {
			case ID_YA_CREADA:

				log_error(logger, "PID: %d trato de crear un segmento con id %d y ya estaba creada",pid, idSegmento);
				send(socket_cliente,&(respuesta),sizeof(uint8_t),0);
				break;

			case OUT_OF_MEMORY:

				log_error(logger, "PID: %d trato de crear un segmento con id %d pero no hay memoria disponible", pid, idSegmento);
				send(socket_cliente,&(respuesta),sizeof(uint8_t),0);
				break;

			case LIMITE_TABLA_SEGMENTOS:

				log_error(logger, "PID: %d trato de crear un segmento con id %d pero no hay mas espacio en su tabla",pid,idSegmento);
				send(socket_cliente,&(respuesta),sizeof(uint8_t),0);
				break;

			case NECESITA_COMPACTACION:

				log_warning(logger, "PID: %d trato de crear un segmento con id %d pero se necesita compactacion para crearlo",pid,idSegmento);
				send(socket_cliente,&(respuesta),sizeof(uint8_t),0);

				op_code respuestaKernel;
				recv(socket_cliente, &(respuestaKernel), sizeof(uint32_t), MSG_WAITALL);

				if(respuestaKernel == COMPACTAR){
					pedido_compactar(socket_cliente);
				}

				break;

			case OK:
				//si esta ok, mando respuesta en un paquete
				t_paquete * paqueteRespuesta = crear_paquete();

				uint32_t base = crearSegmento(pid, idSegmento, tamanio_segmento);

				paqueteRespuesta->codigo_operacion = respuesta;
				paqueteRespuesta->buffer->size = sizeof(uint32_t);
				paqueteRespuesta->buffer->stream = malloc(paqueteRespuesta->buffer->size);
				memcpy(paqueteRespuesta->buffer->stream, &(base), sizeof(uint32_t));
				enviar_paquete(paqueteRespuesta,socket_cliente);
				eliminar_paquete(paqueteRespuesta);
				break;

			default:
				break;
		}



}

void pedido_eliminar_segmento(t_buffer* buffer, int socket_cliente){
	uint32_t pid;
	uint32_t id_segmento;
	void* stream = buffer->stream;
	memcpy(&pid, stream, sizeof(uint32_t));
	stream+=sizeof(uint32_t);
	memcpy(&id_segmento, stream, sizeof(uint32_t));

	eliminarSegmento(pid, id_segmento);

	t_paquete * paquete = crear_paquete();
	int tamanioSerializado = 0;
	
	Proceso* proceso_a_actualizar = encontrarProcesoPorPID(pid);
	void* datosSerializados = serializar_lista_segmentos(proceso_a_actualizar->tablaSegmentos, &tamanioSerializado);

	paquete->buffer->size = tamanioSerializado;
	paquete->buffer->stream = datosSerializados;
	paquete->codigo_operacion = TABLA_SEGMENTOS;

	enviar_paquete(paquete, socket_cliente);

	eliminar_paquete(paquete);
}

void pedido_leer_espacio_usuario(t_buffer* buffer, int socket_cliente, uint8_t idCliente){
	//Tambien llamado mov in
	uint32_t pid;
	uint32_t direccionFisica;
	uint32_t tamanio;
	void* stream = buffer->stream;

	memcpy(&pid, stream, sizeof(uint32_t));
	stream+=sizeof(uint32_t);
	memcpy(&direccionFisica, stream, sizeof(uint32_t));
	stream+=sizeof(uint32_t);
	memcpy(&tamanio, stream, sizeof(uint32_t));

	//Leo el espacio de usuario
	char *valorLeido = malloc(tamanio+1);

	leerEspacioUsuario(valorLeido, pid, direccionFisica, tamanio, idCliente);

	//Envio el valor leido como respuesta
	t_paquete * paquete = crear_paquete();

	paquete->codigo_operacion = OK;
	paquete->buffer->size = tamanio;
	paquete->buffer->stream = malloc(paquete->buffer->size);

	memcpy(paquete->buffer->stream, valorLeido, tamanio);

	enviar_paquete(paquete, socket_cliente);

	free(valorLeido);
	eliminar_paquete(paquete);
}

void pedido_escribir_espacio_usuario(t_buffer* buffer, int socket_cliente, uint8_t idCliente){
	//Tambien llamado mov out
	uint32_t pid;
	uint32_t direccionFisica;
	uint32_t tamanio;
	void* stream = buffer->stream;

	memcpy(&pid, stream, sizeof(uint32_t));
	stream+=sizeof(uint32_t);

	memcpy(&direccionFisica, stream, sizeof(uint32_t));
	stream+=sizeof(uint32_t);

	memcpy(&tamanio, stream, sizeof(uint32_t));
	stream+=sizeof(uint32_t);

	char* valor = malloc(tamanio+1);
	memcpy(valor, stream, tamanio);

	valor[tamanio] = '\0';
	stream += tamanio;

	escribirEspacioUsuario(valor, pid, direccionFisica, tamanio, idCliente);

	op_code respuesta = OK;

	send(socket_cliente,&(respuesta),sizeof(uint32_t),0);

}

void pedido_compactar(int socket_cliente) {

	log_info(logger,"Solicitud de Compactacion");

	compactarSegmentos();

	t_paquete* paquete = crear_paquete();

	for (int i = 0; i < list_size(tablaProcesosGlobal); i++) {
		Proceso* proceso = list_get(tablaProcesosGlobal, i);
		paquete->buffer->size += sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint32_t) * 4 * list_size(proceso->tablaSegmentos);
	}
	
	paquete->buffer->stream = malloc(paquete->buffer->size);

	void * stream = paquete->buffer->stream;
	int desplazamiento = 0;

	for (int i = 0; i < list_size(tablaProcesosGlobal); i++) {

		Proceso* proceso = list_get(tablaProcesosGlobal, i);
		
		// serializar pid del proceso
		memcpy(stream + desplazamiento, &(proceso->pid), sizeof(uint32_t));
		desplazamiento += sizeof(uint32_t);
		
		// serializar cant segmentos del proceso
		uint32_t cantSegmentos = list_size(proceso->tablaSegmentos);
		memcpy(stream + desplazamiento, &cantSegmentos, sizeof(uint32_t));
		desplazamiento += sizeof(uint32_t);

		// serializar segmentos
		for (int i = 0; i < cantSegmentos; i++) {
			t_segmento* segmento = list_get(proceso->tablaSegmentos, i);

			memcpy(stream + desplazamiento, &(segmento->pid), sizeof(uint32_t));
			desplazamiento += sizeof(uint32_t);
			memcpy(stream + desplazamiento, &(segmento->idSegmento), sizeof(uint32_t));
			desplazamiento += sizeof(uint32_t);
			memcpy(stream + desplazamiento, &(segmento->base), sizeof(uint32_t));
			desplazamiento += sizeof(uint32_t);
			memcpy(stream + desplazamiento, &(segmento->tamanio), sizeof(uint32_t));
			desplazamiento += sizeof(uint32_t);
		}
	}

	paquete->buffer->stream = stream;

	paquete->codigo_operacion = OK;

	enviar_paquete(paquete, socket_cliente);
	eliminar_paquete(paquete);
}



