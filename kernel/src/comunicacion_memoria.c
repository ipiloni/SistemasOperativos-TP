#include "comunicacion_memoria.h"

void* procesar_eliminacion_memoria(void *arg) {
	int socket_memoria = *(int *)arg;

	while(1) {
		sem_wait(&eliminar_estructuras);

		enviar_pid_a_memoria(socket_memoria, pcb_en_ejecucion->pid, ELIMINAR_PROCESO);
		log_info(logger,"PID: <%d> - Solicito a memoria la eliminacion de estructuras del proceso",pcb_en_ejecucion->pid);
		sem_post(&estructuras_eliminadas);
	}
}

t_motivo_desalojo crear_segmento(t_instruccion* instruccion_pendiente, int socket_memoria) {
	uint32_t id_segmento = atoi(instruccion_pendiente->parametro1);
	uint32_t tamanio = atoi(instruccion_pendiente->parametro2);

	solicitar_creacion_segmento(socket_memoria, pcb_en_ejecucion->pid, id_segmento, tamanio);

	t_paquete* paquete_respuesta = crear_paquete();

	recv(socket_memoria, &(paquete_respuesta->codigo_operacion), sizeof(uint8_t), MSG_WAITALL);
	log_debug(logger, "Codigo de operacion leido: %s", obtener_cod_op(paquete_respuesta->codigo_operacion));

	int cod_operacion = paquete_respuesta->codigo_operacion;
	t_motivo_desalojo motivo = 0;

	switch (cod_operacion) {
		case NECESITA_COMPACTACION:
			log_warning(logger, "PID: %d trato de crear un segmento con id %d pero se necesita compactacion para crearlo \n",pcb_en_ejecucion->pid,id_segmento);
			int valor_semaforo;
			sem_getvalue(&mutex_operacion_fs_memoria, &valor_semaforo);
			if(valor_semaforo == 0){
				log_info(logger, "Compactación: Esperando Fin de Operaciones de FS");
				 //Fijarte si no hay operaciones de filesystem con memoria y despues pedir la compactacion
			}
			sem_wait(&mutex_operacion_fs_memoria);
			log_info(logger,"Compactación: Se solicitó compactación");
			op_code respuesta = COMPACTAR;
			send(socket_memoria,&(respuesta),sizeof(uint32_t),0);
			recibir_tablas_de_segmentos(socket_memoria);
			sem_post(&mutex_operacion_fs_memoria);
			log_info(logger,"Se finalizó el proceso de compactación");
			break;

		case OK:
			recv(socket_memoria, &(paquete_respuesta->buffer->size), sizeof(int), 0);
			paquete_respuesta->buffer->stream = malloc(paquete_respuesta->buffer->size);
			recv(socket_memoria, paquete_respuesta->buffer->stream, paquete_respuesta->buffer->size, 0);

			t_segmento* segmento = malloc(sizeof(t_segmento));
			segmento->idSegmento= id_segmento;
			segmento->pid = pcb_en_ejecucion->pid;
			segmento->tamanio = tamanio;
			memcpy(&segmento->base,paquete_respuesta->buffer->stream, sizeof(uint32_t));
			list_add(pcb_en_ejecucion->tabla_de_segmentos, segmento);
			pcb_en_ejecucion->program_counter++;
			log_info(logger, "PID: <%d> - Crear Segmento - Id: <%d> - Tamaño: <%d>", segmento->pid, segmento->idSegmento, segmento->tamanio);
			break;
		default:
			motivo = op_code_a_motivo_desalojo(cod_operacion);
			break;
	}
	eliminar_paquete(paquete_respuesta);
	return motivo;
}

void eliminar_segmento(int id_segmento, int socket_memoria) {
	solicitar_eliminacion_segmento(socket_memoria,pcb_en_ejecucion->pid,id_segmento);
	log_info(logger, "PID: <%d> - Eliminar Segmento - Id Segmento: <%d>", pcb_en_ejecucion->pid, id_segmento);

	list_destroy_and_destroy_elements(pcb_en_ejecucion->tabla_de_segmentos, free);

	pcb_en_ejecucion->tabla_de_segmentos = recibir_tabla_segmentos(socket_memoria);
	pcb_en_ejecucion->program_counter++;
}
