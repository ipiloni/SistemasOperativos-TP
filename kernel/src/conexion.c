#include "conexion.h"

t_log* logger;
pthread_t hilo;

void* recibir_cliente(void* socket) {

	int socket_cliente = *((int *)socket);

	if (responder_handshake(socket_cliente) == -1) {
		pthread_exit(NULL);
		return NULL;
	}

	t_paquete* paquete = malloc(sizeof(t_paquete));
	paquete->buffer = malloc(sizeof(t_buffer));

	if(recv(socket_cliente, &(paquete->codigo_operacion), sizeof(uint8_t), 0) == -1) {
		puts("RECV_ERROR");
		pthread_exit(NULL);
	}

	if(recv(socket_cliente, &(paquete->buffer->size), sizeof(uint32_t), 0) == -1) {
		puts("RECV_ERROR");
		pthread_exit(NULL);
	}

	paquete->buffer->stream = malloc(paquete->buffer->size);
	if(recv(socket_cliente, paquete->buffer->stream, paquete->buffer->size, 0) == -1) {
		puts("RECV_ERROR");
		pthread_exit(NULL);
	}

	log_debug(logger, "Codigo de operacion leido: %d", paquete->codigo_operacion);

	switch (paquete->codigo_operacion){
		case PAQUETE_INSTRUCCIONES:
				t_list* lista_instrucciones = list_create();
				uint32_t cantidad_instrucciones = 0;
				memcpy(&cantidad_instrucciones, paquete->buffer->stream, sizeof(uint32_t));
				for(int i = 0; i < cantidad_instrucciones; i++) {
					t_instruccion* instruccion = deserializar_instruccion(paquete->buffer);
					list_add(lista_instrucciones, instruccion);
				}
			  break;
		default:
			  log_warning(logger, "Operación desconocida.\n");
			  pthread_exit(NULL);
		}

	//atender_cliente(paquete, socket_cliente);
	return NULL;
}

int responder_handshake(int socket_cliente) {
	uint32_t handshake;
	uint32_t resultOk = 0;
	uint32_t resultError = -1;

	recv(socket_cliente, &handshake, sizeof(uint32_t), MSG_WAITALL);
	if(handshake == CONSOLA){
		  send(socket_cliente, &resultOk, sizeof(uint32_t), 0);
		  log_debug(logger, "Handshake aceptado.\n");
		  return 0;
	}
	else{
		  send(socket_cliente, &resultError, sizeof(uint32_t), 0);
		  log_debug(logger, "Handshake rechazado.\n");
		  return -1;
	}
}

void atender_cliente(t_paquete* paquete, int socket_cliente) {
	log_info(logger, "Codigo de operacion leido: %d", paquete->codigo_operacion);
	switch (paquete->codigo_operacion){

		case PAQUETE_INSTRUCCIONES:
				log_info(logger, "LO LOGRE!");
				t_list* lista_instrucciones = list_create();
				lista_instrucciones = deserializar_lista_instrucciones(paquete->buffer);
				log_info(logger, "REALMENTE LO HE CONSEGUIDO");
				list_iterate(lista_instrucciones, mostrar_instruccion);
				list_destroy_and_destroy_elements(lista_instrucciones,(void*) instruccion_destroy);
				eliminar_paquete(paquete);
			  break;
		default:
			  log_info(logger, "Operación desconocida.\n");
			  pthread_exit(NULL);
		}
}

void enviar_pid_a_memoria(int socket_memoria, uint32_t pid, op_code cod_op){
	t_paquete* paquete = crear_paquete();
	paquete->codigo_operacion = cod_op;
	paquete->buffer->size = sizeof(uint32_t);
	paquete->buffer->stream = malloc(paquete->buffer->size);
	memcpy(paquete->buffer->stream, &pid, sizeof(uint32_t));
	enviar_paquete(paquete, socket_memoria);
	eliminar_paquete(paquete);
}

void solicitar_creacion_segmento(int socket_memoria, uint32_t pid, uint32_t idSegmento, uint32_t tamanio){
	t_paquete* paquete = crear_paquete();
	paquete->codigo_operacion = CREAR_SEGMENTO;
	paquete->buffer->size = sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint32_t);
	paquete->buffer->stream = malloc(paquete->buffer->size);

	memcpy(paquete->buffer->stream, &pid, sizeof(uint32_t));
	memcpy(paquete->buffer->stream + sizeof(uint32_t), &idSegmento, sizeof(uint32_t));
	memcpy(paquete->buffer->stream + 2 * sizeof(uint32_t), &tamanio, sizeof(uint32_t));

	enviar_paquete(paquete, socket_memoria);
	eliminar_paquete(paquete);
}

void solicitar_eliminacion_segmento(int socket_memoria, uint32_t pid, uint32_t idSegmento){
	t_paquete* paquete = crear_paquete();
	paquete->codigo_operacion = ELIMINAR_SEGMENTO;
	paquete->buffer->size = sizeof(uint32_t) + sizeof(uint32_t);
	paquete->buffer->stream = malloc(paquete->buffer->size);

	memcpy(paquete->buffer->stream, &pid, sizeof(uint32_t));
	memcpy(paquete->buffer->stream + sizeof(uint32_t), &idSegmento, sizeof(uint32_t));
	
	enviar_paquete(paquete, socket_memoria);
	eliminar_paquete(paquete);
}

void solicitar_a_fs(op_code codigo, t_instruccion* instruccion, uint32_t pid, uint32_t puntero, int socket_fs, uint32_t direccion_fisica){
	t_paquete* paquete = crear_paquete();

	paquete->codigo_operacion = codigo;

	paquete->buffer->size = instruccion->parametro1_length + sizeof(uint32_t); //tamanio mas tamanio del tamanio
	
	switch(codigo){
		case RECORTAR_ARCHIVO:
		paquete->buffer->size += sizeof(uint32_t);
		break;
		case LEER_ARCHIVO:
		paquete->buffer->size += 4*sizeof(uint32_t);
		break;
		case ESCRIBIR_ARCHIVO:
		paquete->buffer->size += 4*sizeof(uint32_t);
		break;
		default:
		break;	
	}
	void* stream = malloc(paquete->buffer->size);
	int desplazamiento = 0;

	memcpy(stream + desplazamiento, &(instruccion->parametro1_length), sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);

	memcpy(stream + desplazamiento, instruccion->parametro1, instruccion->parametro1_length);
	desplazamiento += instruccion->parametro1_length;

	switch(codigo){
		case RECORTAR_ARCHIVO:
			{
			uint32_t tamanio = atoi(instruccion->parametro2);
			memcpy(stream + desplazamiento, &(tamanio), sizeof(uint32_t));
			desplazamiento += sizeof(uint32_t);
			}
			break;
		case LEER_ARCHIVO:
			{
			memcpy(stream + desplazamiento, &(direccion_fisica), sizeof(uint32_t));
			desplazamiento += sizeof(uint32_t);
			uint32_t cant_bytes = atoi(instruccion->parametro3);
			memcpy(stream + desplazamiento, &(cant_bytes), sizeof(uint32_t));
			desplazamiento += sizeof(uint32_t);
			memcpy(stream + desplazamiento, &pid, sizeof(uint32_t));
			desplazamiento += sizeof(uint32_t);
			memcpy(stream + desplazamiento, &puntero, sizeof(uint32_t));
			desplazamiento += sizeof(uint32_t);
			}
			break;
		case ESCRIBIR_ARCHIVO:
			{
			memcpy(stream + desplazamiento, &(direccion_fisica), sizeof(uint32_t));
			desplazamiento += sizeof(uint32_t);
			uint32_t cant_bytes = atoi(instruccion->parametro3);
			memcpy(stream + desplazamiento, &(cant_bytes), sizeof(uint32_t));
			desplazamiento += sizeof(uint32_t);
			memcpy(stream + desplazamiento, &pid, sizeof(uint32_t));
			desplazamiento += sizeof(uint32_t);
			memcpy(stream + desplazamiento, &puntero, sizeof(uint32_t));
			desplazamiento += sizeof(uint32_t);
			}
			break;
		default:
		break;	
	}

	paquete->buffer->stream = stream;

	enviar_paquete(paquete, socket_fs);

	eliminar_paquete(paquete);
}

uint32_t preguntar_a_fs_si_existe_archivo_y_abrirlo(t_instruccion* instruccion, int socket_fs){
	t_paquete* paquete = crear_paquete();

	paquete->codigo_operacion = ABRIR_ARCHIVO;

	solicitar_a_fs(ABRIR_ARCHIVO, instruccion, 0, 0, socket_fs, 0);

	eliminar_paquete(paquete);

	uint32_t respuesta;
	recv(socket_fs, &(respuesta), sizeof(uint32_t), MSG_WAITALL);
	
	log_debug(logger, "La respuesta a ABRIR_ARCHIVO fue: %d", respuesta);
	return respuesta;
}

void solicitar_compactacion(int socket_memoria){
	t_paquete* paquete = crear_paquete();
	paquete->codigo_operacion = COMPACTAR;
	paquete->buffer->size = 0;
	paquete->buffer->stream = NULL;
	enviar_paquete(paquete, socket_memoria);
	eliminar_paquete(paquete);
}

void recibir_tablas_de_segmentos(int socket_memoria) {
    t_paquete* paquete = crear_paquete();
    paquete = recibir_paquete(paquete, socket_memoria);

    if(paquete->codigo_operacion == OK){
        void* stream = paquete->buffer->stream;

        for(int i = 0; i < list_size(lista_global_de_pcbs); i++) {
            uint32_t pid = 0;
            memcpy(&pid, paquete->buffer->stream, sizeof(uint32_t));
            paquete->buffer->stream += sizeof(uint32_t);

            t_pcb* pcb = obtener_pcb_por_id(pid);
            list_clean_and_destroy_elements(pcb->tabla_de_segmentos, free);

            uint32_t cantidad_segmentos_de_proceso = 0;
            memcpy(&cantidad_segmentos_de_proceso, paquete->buffer->stream, sizeof(uint32_t));
            paquete->buffer->stream += sizeof(uint32_t);

            for(int j = 0; j < cantidad_segmentos_de_proceso; j++) {
                t_segmento* segmento = deserializar_segmento(paquete->buffer);
                list_add(pcb->tabla_de_segmentos, segmento);
            }

            imprimirSegmentos(pcb->tabla_de_segmentos);
        }
        paquete->buffer->stream = stream;
    }

    eliminar_paquete(paquete);
}

t_pcb* obtener_pcb_por_id(int pid) {
	int esElPCBPorID(t_pcb *pcb) {
	       if (pcb->pid == pid){
	          return true;
	       } else return false;
	 }
     return list_find(lista_global_de_pcbs, (void*) esElPCBPorID);
}
