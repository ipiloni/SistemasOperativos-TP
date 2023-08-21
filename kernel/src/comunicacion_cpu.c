#include "comunicacion_cpu.h"

void* comunicacion_CPU(void* arg) {

	t_conexion_info conexion_info_CPU;
	conexion_info_CPU = *((t_conexion_info*) arg);
	free(arg);
	pthread_detach(pthread_self());

	int socket_memoria = conexion_info_CPU.socket_memoria;
	int socket_CPU = crear_conexion_handshake(conexion_info_CPU.ip, conexion_info_CPU.puerto, KERNEL);

	if(socket_CPU == 1){
		close(socket_CPU);
		puts("socket error"); //perror pero errno seria modificado concurrentemente con lo cual necesitaria mutex para todo y mucho lio
		return NULL;
	}

	while(1) {
		sem_wait(&proceso_running);

		pcb_en_ejecucion->tiempo_llegada_running = time(NULL);

		int flag_replanificar = 0;

		while(flag_replanificar == 0) {

			enviar_contexto_a_cpu(socket_CPU); // una vez

			t_paquete* paquete_respuesta = crear_paquete();

			int bytes_recibidos = recv(socket_CPU, &(paquete_respuesta->codigo_operacion), sizeof(uint8_t), MSG_WAITALL);

			if(bytes_recibidos == -1) {
				close(socket_CPU);
				log_error(logger, "Hubo un error en el contexto de CPU.");
				return NULL;
			}
			else if(bytes_recibidos == 0) {
				close(socket_CPU);
				log_error(logger, "Recibí 0 bytes desde CPU. Probablemente se desconectó.");
				return NULL;
			}

			recv(socket_CPU, &(paquete_respuesta->buffer->size), sizeof(uint32_t), 0);

			paquete_respuesta->buffer->stream = malloc(paquete_respuesta->buffer->size);

			void* stream_original = paquete_respuesta->buffer->stream;

			recv(socket_CPU, paquete_respuesta->buffer->stream, paquete_respuesta->buffer->size, 0);

			if(paquete_respuesta->codigo_operacion == CONTEXTO_DE_EJECUCION) {

				cod_instruccion motivo_desalojo = 0;
				memcpy(&(motivo_desalojo), paquete_respuesta->buffer->stream, sizeof(uint8_t));
				paquete_respuesta->buffer->stream += sizeof(uint8_t);

				//log_debug(logger, "La instrucción a ejecutar es: %s", obtener_motivo(motivo_desalojo));

				memcpy(&(pcb_en_ejecucion->program_counter), paquete_respuesta->buffer->stream, sizeof(uint32_t));
				paquete_respuesta->buffer->stream += sizeof(uint32_t);

				recibir_registros(paquete_respuesta->buffer, pcb_en_ejecucion->registros_cpu);

				pcb_en_ejecucion->instruccion_pendiente = list_get(pcb_en_ejecucion->instrucciones, pcb_en_ejecucion->program_counter);
				t_instruccion* instruccion_pendiente = pcb_en_ejecucion->instruccion_pendiente;

				int comparador_recurso(t_recurso* r) {
					return string_equals_ignore_case(r->nombre_recurso, instruccion_pendiente->parametro1);
				}

				switch(motivo_desalojo) {
				case IO:
					pthread_t thread_bloqueador;
					t_bloqueador_proceso* proceso_bloqueado = malloc(sizeof(t_bloqueador_proceso));
					proceso_bloqueado->tiempo_bloqueado = atoi(instruccion_pendiente->parametro1);
					proceso_bloqueado->pcb_bloqueado = pcb_en_ejecucion;
					pasar_a_blocked(pcb_en_ejecucion, obtener_motivo(motivo_desalojo));
					replanificar(&flag_replanificar);

					pthread_create(&thread_bloqueador, NULL, &bloquear_por_tiempo, proceso_bloqueado);
					break;
				case F_OPEN:
					char* nombre_archivo_solicitado = instruccion_pendiente->parametro1;

					t_archivo_global* archivo_solicitado = buscarArchivoPorNombreGlobal(nombre_archivo_solicitado);

					if(archivo_solicitado==NULL) {
						sem_post(&operacion_fs);
						sem_wait(&puede_replanificar); // para evitar inconsistencias, en realidad no replanifica el f_open en este punto
						sem_wait(&termina_operacion_fs);
						archivo_solicitado = buscarArchivoPorNombreGlobal(nombre_archivo_solicitado);
					}

					if(buscarArchivoPorNombreLocal(nombre_archivo_solicitado, pcb_en_ejecucion->tabla_archivos_abiertos)==NULL){
						//si no esta el archivo localmente lo inicializamos
						t_archivo_proceso* archivo_local = inicializar_archivo_local(nombre_archivo_solicitado);
						list_add(pcb_en_ejecucion->tabla_archivos_abiertos, archivo_local);
					}

					if(estaEnUso(archivo_solicitado)) {
						sem_wait(&mutex_archivos);
						queue_push(archivo_solicitado->cola_esperando, pcb_en_ejecucion);
						sem_post(&mutex_archivos);
						pasar_a_blocked(pcb_en_ejecucion, nombre_archivo_solicitado);

						log_debug(logger,"PID: <%d> agregado a cola de: %s", pcb_en_ejecucion->pid,archivo_solicitado->nombre_archivo);

						replanificar(&flag_replanificar);
					}
					else {
						log_debug(logger,"PID: <%d> puede usar: %s", pcb_en_ejecucion->pid,archivo_solicitado->nombre_archivo);
						archivo_solicitado->bit_de_uso = 1;
						pcb_en_ejecucion->program_counter++;
					}

					log_info(logger, "PID: <%d> - Abrir Archivo: <%s>", pcb_en_ejecucion->pid, nombre_archivo_solicitado);
					break;

				case F_CLOSE:
					nombre_archivo_solicitado = instruccion_pendiente->parametro1;

					if(buscarArchivoPorNombreGlobal(nombre_archivo_solicitado) == NULL){
						log_error(logger, "No se puede cerrar un archivo con ese nombre.");
						replanificar(&flag_replanificar);
						break;
					}

					int comparador_nombre_archivo_global(t_archivo_global * archivo_global) {
						return string_equals_ignore_case(archivo_global->nombre_archivo, nombre_archivo_solicitado);
					}

					int comparador_nombre_archivo_local(t_archivo_proceso * archivo_local) {
						return string_equals_ignore_case(archivo_local->nombre_archivo, nombre_archivo_solicitado);
					}

					list_remove_and_destroy_by_condition(pcb_en_ejecucion->tabla_archivos_abiertos, (void*)comparador_nombre_archivo_local, (void*)eliminar_archivo_local);

					sem_wait(&mutex_archivos);
					t_archivo_global* archivo_global = buscarArchivoPorNombreGlobal(nombre_archivo_solicitado);
					sem_post(&mutex_archivos);

					log_info(logger, "PID: <%d> - Cerrar Archivo: <%s>", pcb_en_ejecucion->pid, archivo_global->nombre_archivo);

					if(queue_is_empty(archivo_global->cola_esperando)){

						sem_wait(&mutex_archivos);
						list_remove_and_destroy_by_condition(tabla_global_de_archivos_abiertos, (void*)comparador_nombre_archivo_global, (void*)eliminar_archivo_global);
						sem_post(&mutex_archivos);

						log_debug(logger, "PID: <%d> - Eliminar <%s> de la tabla global.", pcb_en_ejecucion->pid, nombre_archivo_solicitado);
					} else {

						sem_wait(&mutex_archivos);
						t_pcb* pcb_que_estaba_bloqueado = queue_pop(archivo_global->cola_esperando);
						sem_post(&mutex_archivos);

						archivo_global->bit_de_uso = 0;
						pasar_a_ready(pcb_que_estaba_bloqueado);
						log_debug(logger, "Se libera el proceso <%d> de la cola de bloqueados del archivo <%s>.", pcb_que_estaba_bloqueado->pid, nombre_archivo_solicitado);
					}
					pcb_en_ejecucion->program_counter++;

					break;

				case F_SEEK:
					nombre_archivo_solicitado = instruccion_pendiente->parametro1;
					sem_wait(&mutex_archivos);
					t_archivo_proceso* archivo_proceso = buscarArchivoPorNombreLocal(nombre_archivo_solicitado, pcb_en_ejecucion->tabla_archivos_abiertos);
					sem_post(&mutex_archivos);
					archivo_proceso->puntero = atoi(instruccion_pendiente->parametro2);
					log_info(logger, "PID: <%d> - Actualizar puntero Archivo: <%s> - Puntero <%d>", pcb_en_ejecucion->pid, archivo_proceso->nombre_archivo, archivo_proceso->puntero);
					pcb_en_ejecucion->program_counter++;
					break;

				case F_TRUNCATE:
					sem_post(&operacion_fs);
					sem_wait(&puede_replanificar);

					replanificar(&flag_replanificar);
					break;

				case F_READ:	//no sacar estas llaves. Aclaración: han sido sacadas
					direccion_fisica = 0;
					recv(socket_CPU, &direccion_fisica, sizeof(uint32_t),MSG_WAITALL);

					sem_post(&operacion_fs);
					sem_wait(&puede_replanificar);

					replanificar(&flag_replanificar);
					break;

				case F_WRITE:
					direccion_fisica = 0;
					recv(socket_CPU, &direccion_fisica, sizeof(uint32_t),MSG_WAITALL);

					sem_post(&operacion_fs);
					sem_wait(&puede_replanificar);


					replanificar(&flag_replanificar);
					break;

				case WAIT:
					t_recurso* recurso_solicitado = list_find(lista_recursos, (void*)comparador_recurso);

					if(recurso_solicitado == NULL){
						log_error(logger,"PID: <%d> - Se solicito un wait de un recurso invalido", pcb_en_ejecucion->pid);
						terminar_proceso_en_ejecucion(RECURSO_INVALIDO, socket_memoria);
						replanificar(&flag_replanificar);
					}
					else if(recurso_solicitado->instancias > 0) {
						(recurso_solicitado->instancias)--;
						(pcb_en_ejecucion->program_counter)++;
						log_info(logger,"PID: <%d> - Wait: <%s> - Instancias: <%d>", pcb_en_ejecucion->pid, recurso_solicitado->nombre_recurso, recurso_solicitado->instancias);
					}
					else if (recurso_solicitado->instancias <= 0) {
						queue_push(recurso_solicitado->procesos_bloqueados, pcb_en_ejecucion);
						pasar_a_blocked(pcb_en_ejecucion, recurso_solicitado->nombre_recurso);

						replanificar(&flag_replanificar);
					}
					break;
				case SIGNAL:
					t_recurso* recurso_liberado = list_find(lista_recursos, (void*)comparador_recurso);

					if(recurso_liberado == NULL) {
						log_error(logger,"PID: <%d> - Se solicito un signal de un recurso invalido", pcb_en_ejecucion->pid);
						terminar_proceso_en_ejecucion(RECURSO_INVALIDO, socket_memoria);
						replanificar(&flag_replanificar);
						break;
					}
					recurso_liberado->instancias++;
					log_info(logger, "PID: <%d> - Signal: <%s> - Instancias: <%d>", pcb_en_ejecucion->pid, recurso_liberado->nombre_recurso, recurso_liberado->instancias);
					if(!queue_is_empty(recurso_liberado->procesos_bloqueados)) {
						t_pcb* pcb_desbloqueado = queue_pop(recurso_liberado->procesos_bloqueados);
						log_info(logger,"PID: <%d> - Se saca de la cola de procesos bloqueados del recurso: <%s>", pcb_desbloqueado->pid, recurso_liberado->nombre_recurso);
						pasar_a_ready(pcb_desbloqueado);
					}
					pcb_en_ejecucion->program_counter++;
					break;
				case CREATE_SEGMENT:
					t_motivo_desalojo motivo = crear_segmento(instruccion_pendiente, socket_memoria);
					if (motivo != 0) {
						terminar_proceso_en_ejecucion(motivo, socket_memoria);
						replanificar(&flag_replanificar);
					}
					break;
				case DELETE_SEGMENT:
					int id_segmento = atoi(instruccion_pendiente->parametro1);
					eliminar_segmento(id_segmento, socket_memoria);
					break;
				case YIELD:
					pcb_en_ejecucion->real_ejecucion_anterior = difftime(time(NULL), pcb_en_ejecucion->tiempo_llegada_running)*1000;
					calcular_estimado_ejecucion(pcb_en_ejecucion);

					pasar_a_ready(pcb_en_ejecucion);

					(pcb_en_ejecucion->program_counter)++;
					replanificar(&flag_replanificar);
					break;
				default:
					motivo = op_code_a_motivo_desalojo(motivo_desalojo);
					terminar_proceso_en_ejecucion(motivo, socket_memoria);
					replanificar(&flag_replanificar);
				}
			}
		paquete_respuesta->buffer->stream = stream_original;
		eliminar_paquete(paquete_respuesta);
		}
	}
	return NULL;
}

void recibir_registros(t_buffer* buffer, t_registros_cpu* registros) {
//	t_registros_cpu* registros = malloc(sizeof(t_registros_cpu));

	memcpy(registros->AX, buffer->stream, 4);
	buffer->stream += 4;

	memcpy(registros->BX, buffer->stream, 4);
	buffer->stream += 4;

	memcpy(registros->CX, buffer->stream, 4);
	buffer->stream += 4;

	memcpy(registros->DX, buffer->stream, 4);
	buffer->stream += 4;

	memcpy(registros->EAX, buffer->stream, 8);
	buffer->stream += 8;

	memcpy(registros->EBX, buffer->stream, 8);
	buffer->stream += 8;

	memcpy(registros->ECX, buffer->stream, 8);
	buffer->stream += 8;

	memcpy(registros->EDX, buffer->stream, 8);
	buffer->stream += 8;

	memcpy(registros->RAX, buffer->stream, 16);
	buffer->stream += 16;

	memcpy(registros->RBX, buffer->stream, 16);
	buffer->stream += 16;

	memcpy(registros->RCX, buffer->stream, 16);
	buffer->stream += 16;

	memcpy(registros->RDX, buffer->stream, 16);
	buffer->stream += 16;

//	return registros;
}


