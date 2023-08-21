#include "main.h"

t_queue* cola_new;
t_queue* cola_ready;
t_list* lista_global_de_pcbs;
t_list* lista_pcbs;
t_list* lista_recursos;
sem_t mutex_cola_new;
sem_t mutex_cola_ready;
sem_t mutex_lista_pcbs;
sem_t mutex_archivos;
sem_t proceso_nuevo;
sem_t sem_corto_plazo;
sem_t proceso_ready;
sem_t sem_replanificar;
sem_t proceso_running;
sem_t grado_multiprogramacion_disponible;
sem_t tope_ejecucion;
sem_t cantidad_procesos;
sem_t operacion_fs;
sem_t termina_operacion_fs;
sem_t puede_replanificar;
sem_t mutex_operacion_fs_memoria;
sem_t eliminar_estructuras;
sem_t estructuras_eliminadas;

t_pcb* pcb_en_ejecucion;

t_list* tabla_global_de_archivos_abiertos;

pthread_mutex_t mutex_proceso_finalizado = PTHREAD_MUTEX_INITIALIZER; //TODO reemplazar los sem_t mutex por pthread_mutex_t
pthread_cond_t finaliza_proceso = PTHREAD_COND_INITIALIZER;
t_info_finalizacion proceso_finalizado;

int pid_actual;
int grado_multiprogramacion_actual;
int grado_max_multiprogramacion;
int sin_memoria;
uint32_t direccion_fisica;
t_config* kernel_config;

char* lista_pids(void) {
	char* lista = string_new();
	char* pid;
	t_pcb* pcb;
	sem_wait(&mutex_lista_pcbs);
	for(int i = 0; i<list_size(lista_pcbs); i++) {
		pcb = list_get(lista_pcbs, i);
		pid = string_itoa(pcb->pid);
		if(pcb->pid == ultimo_pid())
			string_append(&lista, pid);
		else
			string_append_with_format(&lista, "%s,", pid);
		free(pid);
	}
	sem_post(&mutex_lista_pcbs);

	return lista;
}

int ultimo_pid(void) {
	t_pcb* ultimo_pcb = list_get(lista_pcbs, list_size(lista_pcbs)-1);
	return ultimo_pcb->pid;
}


int main(int argc, char** argv) {

	if (argc < 2)
		return -1;

	char* puerto_escucha;

	pid_actual = 0;
	grado_multiprogramacion_actual = 0;
	cola_new = queue_create();
	lista_pcbs = list_create();
	lista_recursos = list_create();
	lista_global_de_pcbs = list_create();
	proceso_finalizado.pid = -1;
	proceso_finalizado.motivo_desalojo = MOTIVO_DESCONOCIDO;

	kernel_config = config_create(argv[1]); //memory leak, todavia no se donde se liberarian todos los malloc

	sem_init(&mutex_cola_new, 0, 1);
	sem_init(&mutex_cola_ready, 0, 1);
	sem_init(&mutex_lista_pcbs, 0, 1);
	sem_init(&mutex_archivos, 0, 1);
	sem_init(&proceso_nuevo, 0, 0);
	sem_init(&proceso_ready, 0, 0);
	sem_init(&sem_replanificar, 0, 1);
	sem_init(&sem_corto_plazo, 0, 0);
	sem_init(&proceso_running, 0, 0);
	int grado_max_multiprogramacion = config_get_int_value(kernel_config, "GRADO_MAX_MULTIPROGRAMACION");
	sem_init(&grado_multiprogramacion_disponible, 0, grado_max_multiprogramacion);
	sem_init(&tope_ejecucion, 0, 1);
	sem_init(&operacion_fs, 0, 0);
	sem_init(&termina_operacion_fs, 0, 0);
	sem_init(&puede_replanificar, 0, 0);
	sem_init(&mutex_operacion_fs_memoria, 0, 1);
	sem_init(&eliminar_estructuras, 0, 0);
	sem_init(&estructuras_eliminadas, 0, 0);

	logger = log_create("kernel.log", "Kernel", 1, LOG_LEVEL_DEBUG);

	char** array_recursos = config_get_array_value(kernel_config,"RECURSOS");
	char** array_instancias_recursos = config_get_array_value(kernel_config,"INSTANCIAS_RECURSOS");
	iniciar_recursos(array_recursos, array_instancias_recursos);

	int cantidad_recursos = list_size(lista_recursos);
	log_info(logger, "Cantidad de recursos iniciados: %d",cantidad_recursos);

	char* ip_memoria = config_get_string_value(kernel_config, "IP_MEMORIA");
	char* puerto_memoria = config_get_string_value(kernel_config, "PUERTO_MEMORIA");
	t_conexion_info* conexion_info_memoria = malloc(sizeof(t_conexion_info));
	strcpy(conexion_info_memoria->ip, ip_memoria);
	strcpy(conexion_info_memoria->puerto, puerto_memoria);

	char* ip_filesystem = config_get_string_value(kernel_config, "IP_FILESYSTEM");
	char* puerto_filesystem = config_get_string_value(kernel_config, "PUERTO_FILESYSTEM");
	t_conexion_info* conexion_info_filesystem = malloc(sizeof(t_conexion_info));
	strcpy(conexion_info_filesystem->ip, ip_filesystem);
	strcpy(conexion_info_filesystem->puerto, puerto_filesystem);

	char* ip_CPU = config_get_string_value(kernel_config, "IP_CPU");
	char* puerto_CPU = config_get_string_value(kernel_config, "PUERTO_CPU");
	t_conexion_info* conexion_info_CPU = malloc(sizeof(t_conexion_info));
	strcpy(conexion_info_CPU->ip, ip_CPU);
	strcpy(conexion_info_CPU->puerto, puerto_CPU);

	int socket_memoria = crear_conexion_handshake(ip_memoria, puerto_memoria, KERNEL);
	if(socket_memoria == -1) {
		close(socket_memoria);
		log_error(logger, "socket error");
		return -1;
	}

	// ESTA REEEEE BUENO ESTO
	conexion_info_CPU->socket_memoria = socket_memoria;
	// LO HICIERON MARCOS, JULIAN Y JAVIER. BESOSX

	pthread_t thread_planificador_largo_plazo;
	pthread_t thread_planificador_corto_plazo;
	pthread_t thread_comunicacion_CPU;
	pthread_t thread_comunicacion_FS;

	pthread_create(&thread_planificador_largo_plazo, NULL, &planificador_largo_plazo, &socket_memoria);
	pthread_detach(thread_planificador_largo_plazo);

	pthread_create(&thread_planificador_corto_plazo, NULL, &planificador_corto_plazo, NULL);
	pthread_detach(thread_planificador_corto_plazo);

	pthread_create(&thread_comunicacion_CPU, NULL, &comunicacion_CPU, conexion_info_CPU);

	pthread_create(&thread_comunicacion_FS, NULL, &comunicacion_FS, conexion_info_filesystem);

	puerto_escucha = config_get_string_value(kernel_config, "PUERTO_ESCUCHA");

	int socket_servidor = iniciar_servidor(puerto_escucha);

	log_info(logger, "Kernel listo para recibir en puerto: %s \n",puerto_escucha);

	int* socket_consola;
	pthread_t thread_comunicacion_consola;

	while(1) {
		socket_consola = malloc(sizeof(int));
		*socket_consola = accept(socket_servidor, NULL, NULL);

		if(*socket_consola == -1) {
			perror("accept");
			continue;
		}

		pthread_create(&thread_comunicacion_consola, NULL, comunicacion_consola, socket_consola);
	}
}


void* comunicacion_consola(void* arg) {
	 int socket_cliente;
	 socket_cliente = *((int *) arg);
	 free(arg);

	 pthread_detach(pthread_self());

	if (responder_handshake(socket_cliente) == -1) {
		log_info(logger, "HANDSHAKE FALLIDO\n");
		return NULL; //HANDSHAKE_FALLIDO
	}

	log_info(logger, "Se conecto un cliente!");

	t_paquete* paquete = crear_paquete();

	paquete = recibir_paquete(paquete,socket_cliente);

	switch (paquete->codigo_operacion){
		case PAQUETE_INSTRUCCIONES:
			t_list* lista_instrucciones = list_create();
			uint32_t cantidad_instrucciones = 0;

			memcpy(&cantidad_instrucciones, paquete->buffer->stream, sizeof(uint32_t));

			void* stream_original = paquete->buffer->stream;
			paquete->buffer->stream += sizeof(uint32_t); // desplazo desde el buffer para que se mantenga el desplazamiento entre las distintas instrucciones que deserializo, preguntar por mejor manera de resolverlo

			for(int i = 0; i < cantidad_instrucciones; i++) {
				t_instruccion* instruccion = deserializar_instruccion(paquete->buffer);
				list_add(lista_instrucciones, instruccion);
			}

			paquete->buffer->stream = stream_original;
			eliminar_paquete(paquete);

			//Creamos PCB y pusheamos a la cola de NEW
			t_pcb* pcb = crear_pcb(lista_instrucciones);

			sem_wait(&mutex_cola_new);
			queue_push(cola_new,pcb);
			sem_post(&mutex_cola_new);
			pcb->estado = NEW;
			log_info(logger, "Se crea el proceso <%d> en <%s>", pcb->pid, estado_a_string(pcb->estado));
			sem_post(&proceso_nuevo);

			pthread_mutex_lock(&mutex_proceso_finalizado);
			while (proceso_finalizado.pid != pcb->pid) {
				pthread_cond_wait(&finaliza_proceso, &mutex_proceso_finalizado); // esperar a la notificación para el proceso asociado
			}
			pthread_mutex_unlock(&mutex_proceso_finalizado);

			uint8_t respuesta_proceso = proceso_finalizado.motivo_desalojo;
			send(socket_cliente, &respuesta_proceso, sizeof(uint8_t), 0);
			free(pcb);

		    break;
		default:
			  log_info(logger, "Operación desconocida.\n");
			  return NULL;
		}

	return NULL;
}

char* traductor_motivo_desalojo(t_motivo_desalojo motivo_desalojo){

	switch(motivo_desalojo){
		case MOTIVO_DESCONOCIDO: return "UNKNOWN_MOTIVE"; break;
		case SEGMENTATION_FAULT: return "SEG_FAULT"; break;
		case FINALIZA_PROCESO: return "SUCCESS"; break;
		case SIN_MEMORIA: return "OUT_OF_MEMORY"; break;
		case RECURSO_INVALIDO: return "INVALID_RESOURCE"; break;
		default: return "xxd";
	}
}

void iniciar_recursos(char** recursos,char** instancias_recursos){
	int cantidad_recursos = string_array_size(recursos);

	for(int i=0;i<cantidad_recursos;i++){
		t_recurso* nuevo_recurso = malloc(sizeof(t_recurso));
		nuevo_recurso->nombre_recurso = recursos[i];
		nuevo_recurso->instancias = atoi(instancias_recursos[i]);
		nuevo_recurso->procesos_bloqueados = queue_create();

		list_add(lista_recursos,nuevo_recurso);
		log_info(logger, "Recurso: %s - Instancias: %d",nuevo_recurso->nombre_recurso,nuevo_recurso->instancias);
	}

}

void terminar_programa(int socket, t_config* config, t_log* logger, t_list* lista_instrucciones, t_paquete* paquete) {
	close(socket);
	config_destroy(config);
	list_destroy_and_destroy_elements(lista_instrucciones,(void*) instruccion_destroy);
	eliminar_paquete(paquete);
	log_destroy(logger);
	list_destroy_and_destroy_elements(lista_pcbs,(void*) pcb_destroy);
	//queue_destroy_and_destroy_elements(cola_new,(void*) pcb_destroy);
}

