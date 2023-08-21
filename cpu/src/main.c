#include "main.h"

/* Por ahora, CPU no utiliza hilos para su comunicacion con memoria y kernel */

t_log* logger;
t_config* config;
t_contexto* contexto;
int tamanio_max_segmento;
int se_necesita_cambio_de_contexto;

int main(int argc, char** argv) {

	if (argc < 2)
		return -1;

	int socket_memoria;
	char* puerto_escucha;

	logger = iniciar_logger();
	config = config_create(argv[1]);

	//log_info(logger, "AX: %.*s", 4, contexto->registros_cpu->AX);

	/* Conexion a Memoria */
	log_info(logger, "Intento de conexion a Memoria");
	socket_memoria = conectarse_a_memoria();

	puerto_escucha = config_get_string_value(config, "PUERTO_ESCUCHA");
	tamanio_max_segmento = config_get_int_value(config, "TAM_MAX_SEGMENTO");
	/* Inicio de Servidor */

	int server_cpu = iniciar_servidor(puerto_escucha);
	log_info(logger, "CPU listo para recibir en puerto: %s \n",puerto_escucha);
	/* Espera de comunicacion con Kernel */
	int socket_kernel = esperar_kernel(server_cpu);

	while(!comunicarse_a_kernel(socket_kernel, socket_memoria));

}

t_log* iniciar_logger(void)
{
	t_log* nuevo_logger;
	nuevo_logger = log_create("cpu.log", "Modulo CPU", 1, LOG_LEVEL_DEBUG);
	return nuevo_logger;
}

t_config* iniciar_config(void)
{
	t_config* nuevo_config;
	nuevo_config = config_create("./cpu.config");
	return nuevo_config;
}

void terminar_programa(int conexion, t_log* logger, t_config* config)
{
	log_destroy(logger);
	config_destroy(config);
	liberar_conexion(conexion);
	free(contexto->instrucciones);
	free(contexto->registros_cpu);
	free(contexto);
}

void inicializar_contexto(t_contexto* contexto) {
	contexto->instrucciones = list_create();
	contexto->tabla_de_segmentos = list_create();
	contexto->registros_cpu = malloc(sizeof(t_registros_cpu));
	inicializar_registros_cpu(contexto->registros_cpu); // se puede hacer con un solo memset
}

void eliminar_contexto(t_contexto* contexto) {
	list_destroy_and_destroy_elements(contexto->instrucciones, (void*)instruccion_destroy);
	list_destroy_and_destroy_elements(contexto->tabla_de_segmentos, (void*)free);
	free(contexto->registros_cpu);
	free(contexto);
}

uint32_t obtener_dir_fisica(char* dir_logica, uint32_t bytes) {
	uint32_t direccion = atoi(dir_logica);

	int segmento_id = obtener_id_segmento(dir_logica);

	int desplazamiento_logico = obtener_desplazamiento_segmento(direccion);

	if(encontrarSegmentoPorIDYTabla(segmento_id, contexto->tabla_de_segmentos) == NULL){
		log_error(logger, "PID: %d - Error SEG_FAULT - No existe segmento - Segmento: %d - Offset: %d", contexto->pid, segmento_id, desplazamiento_logico);
		return -1;
	}

	//  el desplazamiento dentro del segmento (desplazamiento_segmento) sumado al tamaño a leer / escribir,
	//	sea mayor al tamaño del segmento

	t_segmento* segmento = encontrarSegmentoPorIDYTabla(segmento_id, contexto->tabla_de_segmentos);
	if(desplazamiento_logico+bytes > segmento->tamanio){
		log_error(logger, "PID: %d - Error SEG_FAULT - Segmento: %d - Offset: %d - Tamaño: %d ", contexto->pid, segmento_id, desplazamiento_logico, segmento->tamanio);
		return -1;
	}
	uint32_t direccion_fisica = segmento->base + desplazamiento_logico;

	return direccion_fisica;
}

uint32_t obtener_num_segmento(uint32_t direccion) {
	return (direccion / tamanio_max_segmento);
}

uint32_t obtener_desplazamiento_segmento(uint32_t direccion) {
	return (direccion % tamanio_max_segmento);
}
