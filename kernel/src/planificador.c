#include "planificador.h"

void* planificador_largo_plazo(void* arg) { //TODO
	int socket_memoria;
	socket_memoria = *((int*) arg);
	//free(arg); EN CASO DE EMERGENCIA, CULPAR A MARCOS
	pthread_detach(pthread_self());

	 while(1){
		sem_wait(&proceso_nuevo);
		sem_wait(&grado_multiprogramacion_disponible);

		sem_wait(&mutex_cola_new);
		t_pcb* pcb = queue_pop(cola_new);
		sem_post(&mutex_cola_new);

		enviar_pid_a_memoria(socket_memoria, pcb->pid, CREAR_PROCESO);

		pcb->tabla_de_segmentos = recibir_tabla_segmentos(socket_memoria);

		log_debug(logger, "Estructuras inicializadas PID: %d ", pcb->pid);
		//imprimirSegmentos(pcb->tabla_de_segmentos);

		pasar_a_ready(pcb);

		pcb->estimacion_proxima_rafaga = config_get_double_value(kernel_config, "ESTIMACION_INICIAL");
		calcular_estimado_ejecucion(pcb);
		//log_debug(logger, "Estimacion proxima rafaga PID %d = %lf", pcb->pid, pcb->estimacion_proxima_rafaga);

		list_add(lista_global_de_pcbs, pcb);

		sem_post(&sem_corto_plazo);
	}
	return NULL;
}


void* planificador_corto_plazo(void* arg) { //TODO
	sem_wait(&sem_corto_plazo);
	char* algoritmo_planificacion = config_get_string_value(kernel_config, "ALGORITMO_PLANIFICACION");
	log_info(logger, "Algoritmo: %s", algoritmo_planificacion);
	if(strcmp(algoritmo_planificacion, "FIFO") == 0) {
		//FIFO
		while(1) {
			sem_wait(&sem_replanificar);
			sem_wait(&tope_ejecucion); // review
			sem_wait(&cantidad_procesos);


			sem_wait(&mutex_lista_pcbs);
			pcb_en_ejecucion = list_remove_by_condition(lista_pcbs, (void*)pcb_cargada_en_lista); // hay que armar un semaforo para que no intente quitar un elemento si ya no hay mas, un contador
			sem_post(&mutex_lista_pcbs);

			cambiar_estado_proceso(pcb_en_ejecucion, RUNNING);
			sem_post(&proceso_running);
		}
	}
	else if (strcmp(algoritmo_planificacion, "HRRN") == 0) {
		//HRRN
		while(1) {
			sem_wait(&sem_replanificar);
			sem_wait(&tope_ejecucion);
			sem_wait(&cantidad_procesos);

			sem_wait(&mutex_lista_pcbs);
			list_iterate(lista_pcbs, (void*)calcular_ratio_response);
			pcb_en_ejecucion = (t_pcb*) list_get_maximum(lista_pcbs, (void *) pcbs_highest_ratio_response);
			list_remove_element(lista_pcbs, pcb_en_ejecucion);
			sem_post(&mutex_lista_pcbs);

			cambiar_estado_proceso(pcb_en_ejecucion, RUNNING);
			sem_post(&proceso_running);
		}
	}
	else {
		log_error(logger, "Algoritmo inválido.");
		return NULL;
	}

	return NULL;
}

t_pcb* crear_pcb(t_list* lista_instrucciones){
	t_pcb* pcb = malloc(sizeof(t_pcb));

	pid_actual++;

	pcb->pid = pid_actual;
	pcb->estado = NEW;
	pcb->cantidad_instrucciones = list_size(lista_instrucciones);
	pcb->instrucciones = lista_instrucciones;
	pcb->program_counter = 0;
	pcb->registros_cpu = malloc(sizeof(t_registros_cpu));
	memset(pcb->registros_cpu, 0, sizeof(t_registros_cpu));
	pcb->estimacion_proxima_rafaga = 0;
	pcb->real_ejecucion_anterior = 0;
	pcb->tabla_archivos_abiertos = list_create();
	pcb->real_ejecucion_anterior = 0;
	//pcb->tabla_de_segmentos = list_create();
   //Falta tiempo llegada a ready
	return pcb;
}

char* estado_a_string(estado_proceso estado) {
	switch (estado) {
	case NEW:
		return "NEW";
	case READY:
		return "READY";
	case RUNNING:
		return "RUNNING";
	case BLOCKED:
		return "BLOCKED";
	case ESTADO_EXIT:
		return "EXIT";
	default:
		return "ESTADO_INVALIDO";
	}
}

void pcb_destroy(t_pcb* pcb) {
	list_destroy_and_destroy_elements(pcb->instrucciones, (void*)instruccion_destroy);
	list_destroy_and_destroy_elements(pcb->tabla_de_segmentos, free);
	list_destroy_and_destroy_elements(pcb->tabla_archivos_abiertos, free);
	eliminar_registros(pcb->registros_cpu);
	//temporal_destroy(pcb->tiempo_en_ready);
}

void eliminar_registros(t_registros_cpu* registros) {
	free(registros);
}

bool pcb_cargada_en_lista(t_pcb* pcb) {
	if(pcb != NULL)
		return true;
	else
		return false;
}

//funciones de HRRN
void* pcbs_highest_ratio_response (t_pcb* pcb1, t_pcb* pcb2) {
    return pcb1->ratio_response >= pcb2->ratio_response? pcb1 : pcb2;
}

void calcular_ratio_response (t_pcb* pcb) {
   double tiempo_esperado = difftime(time(NULL), pcb->tiempo_llegada_a_ready)*1000;
   log_debug(logger, "Tiempo esperado del proceso con PID %d = %lf", pcb->pid, tiempo_esperado);
   log_debug(logger, "Tiempo estimado de ejecucion para proceso con PID %d = %lf", pcb->pid, pcb->estimacion_proxima_rafaga);
   pcb->ratio_response = 1.0 + tiempo_esperado/pcb->estimacion_proxima_rafaga;
   //log_debug(logger,"el ratio response del pid %d es %lf",pcb->pid, pcb->ratio_response);
}

void calcular_estimado_ejecucion(t_pcb* pcb) {

	double alfa = config_get_double_value(kernel_config, "HRRN_ALFA");

	pcb->estimacion_proxima_rafaga = (pcb->estimacion_proxima_rafaga) * alfa + (pcb->real_ejecucion_anterior) * (1-alfa);
}
// Tiempo estimado proxima rafaga = Tiempo estimado rafaga anterior*alfa + tiempo real de ejecucion anterior * (1-alfa)

void desalojar_proceso_en_ejecucion(void) { // no es necesario pasar pcb porque va a ser el de ejecucion, variable global.
	// podemos desalojar y pasarlo a ready o podemos desalojar y pasarlo a blocked o podemos desalojar y pasarlo a exit.
	// aca deberiamos dejar el codigo comun entre esas 3 situaciones
	// estaria bueno logear aca tambien, pero es complicado sacar los distintos casos, mejor hacer una funcion a parte con toda la logica de loggear un cambio de estado
	pcb_en_ejecucion->real_ejecucion_anterior = difftime(time(NULL), pcb_en_ejecucion->tiempo_llegada_running)*1000;
	calcular_estimado_ejecucion(pcb_en_ejecucion);

}

void pasar_a_ready(t_pcb* pcb) {
	sem_wait(&mutex_lista_pcbs);
	list_add(lista_pcbs, pcb);
	sem_post(&mutex_lista_pcbs);

	cambiar_estado_proceso(pcb, READY);

	pcb->tiempo_llegada_a_ready = time(NULL);

	char* algoritmo_planificacion = config_get_string_value(kernel_config, "ALGORITMO_PLANIFICACION");

	char* pids = lista_pids();
	log_info(logger, "Cola Ready <%s>: [%s]", algoritmo_planificacion, pids);
	free(pids);

	sem_post(&cantidad_procesos);
}

t_motivo_desalojo op_code_a_motivo_desalojo(op_code codigo_operacion) {
	switch(codigo_operacion) {
		case OUT_OF_MEMORY:
			return SIN_MEMORIA;
		case ID_YA_CREADA:
			return SEGMENTO_YA_CREADO;
		case LIMITE_TABLA_SEGMENTOS:
			return SIN_ESPACIO_EN_TABLA;
		case SEG_FAULT:
			return SEGMENTATION_FAULT;
		case EXIT:
			return FINALIZA_PROCESO;
		default:
			return MOTIVO_DESCONOCIDO;
	}
}

void cambiar_estado_proceso(t_pcb* pcb, estado_proceso nuevo_estado) {
	char* estado_anterior = estado_a_string(pcb->estado);
	pcb->estado = nuevo_estado;
	log_info(logger, "PID: <%d> - Estado Anterior: <%s> - Estado Actual: <%s>", pcb->pid, estado_anterior, estado_a_string(pcb->estado));
}

void replanificar(int* flag_replanificar) {
	sem_post(&tope_ejecucion);
	sem_post(&sem_replanificar);
	*flag_replanificar = 1;
}

void pasar_a_blocked(t_pcb* pcb, char* motivo_bloqueo) {
	pcb->real_ejecucion_anterior = difftime(time(NULL), pcb->tiempo_llegada_running)*1000;
	calcular_estimado_ejecucion(pcb);
	log_info(logger, "PID: <%d> - Bloqueado por: %s", pcb->pid, motivo_bloqueo);
	cambiar_estado_proceso(pcb, BLOCKED);
}

void* bloquear_por_tiempo (void* arg) {
	t_bloqueador_proceso proceso;
	proceso = *((t_bloqueador_proceso *) arg);
	free(arg);
	pthread_detach(pthread_self());

	log_info(logger, "PID: <%d> - Ejecuta IO: %d", proceso.pcb_bloqueado->pid, proceso.tiempo_bloqueado);
	sleep(proceso.tiempo_bloqueado);
	(proceso.pcb_bloqueado->program_counter)++;

	pasar_a_ready(proceso.pcb_bloqueado);
	return NULL;
}

void terminar_proceso_en_ejecucion(t_motivo_desalojo motivo_desalojo, int socket_memoria){
	enviar_pid_a_memoria(socket_memoria, pcb_en_ejecucion->pid, ELIMINAR_PROCESO);
	log_info(logger,"PID: <%d> - Solicito a memoria la eliminacion de estructuras del proceso",pcb_en_ejecucion->pid);

	cambiar_estado_proceso(pcb_en_ejecucion, ESTADO_EXIT);
	log_info(logger, "Finaliza el proceso <%d> - Motivo: <%s>", pcb_en_ejecucion->pid, traductor_motivo_desalojo(motivo_desalojo));

	list_remove_element(lista_global_de_pcbs, pcb_en_ejecucion);

	pcb_destroy(pcb_en_ejecucion);

	pthread_mutex_lock(&mutex_proceso_finalizado);
	proceso_finalizado.pid = pcb_en_ejecucion->pid;
	proceso_finalizado.motivo_desalojo = motivo_desalojo;
	pthread_cond_broadcast(&finaliza_proceso); // notificar a todos los hilos a la espera
	pthread_mutex_unlock(&mutex_proceso_finalizado);

	//hay que poner semaforo para no traer el id del pcb en ejecucion? TODO

	sem_post(&grado_multiprogramacion_disponible);
}
