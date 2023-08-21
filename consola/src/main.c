#include "main.h"

t_log* logger;

int main(int argc, char** argv) {
	/*El módulo consola, será el punto de partida de los diferentes procesos que se crearán dentro de esta simulación de un kernel. Cada instancia de dicho módulo ejecutará un solo proceso.
Para poder iniciar una consola, la misma deberá poder recibir por parámetro los siguientes datos:
Archivo de configuración
Archivo de pseudocódigo con las instrucciones a ejecutar.
	 */
	if (argc < 2)
		return -1;

	int conexion_kernel;
	char* ip;
	char* puerto;


	t_config* config;

	logger = iniciar_logger();
	config = iniciar_config(argv[2]);

	FILE *f_instrucciones = fopen(argv[1], "r");
	t_list* lista_instrucciones = list_create();

	cargar_instrucciones(f_instrucciones,lista_instrucciones);

	uint32_t cantidad_instrucciones = list_size(lista_instrucciones);

	log_info(logger, "Tamaño de lista: %d \n", cantidad_instrucciones);

	list_iterate(lista_instrucciones, mostrar_instruccion); //TEST

	//implementando serializacion desde 0 segun guia
	t_instruccion* instruccion;

	t_buffer* buffer = malloc(sizeof(t_buffer));

	buffer->stream = NULL;

	buffer->size = sizeof(int); // el primer elemento sera un int que representa cantidad_instrucciones

	for(int i =0; i < cantidad_instrucciones;i++){ // cargando buffer->size con el tamaño de todas las instrucciones

		instruccion = list_get(lista_instrucciones,i);
		buffer->size += sizeof(uint32_t) * 4 + instruccion->nombre_length + instruccion->parametro1_length + instruccion->parametro2_length + instruccion->parametro3_length;
	}

	void* stream = malloc(buffer->size);
	int offset = 0; // Desplazamiento

	memcpy(stream, &cantidad_instrucciones, sizeof(uint32_t));
	offset += sizeof(uint32_t);

	for(int i = 0; i < cantidad_instrucciones; i++) { //cargando stream del buffer

		instruccion = list_get(lista_instrucciones,i);

		memcpy(stream + offset, &(instruccion->nombre_length), sizeof(uint32_t));
		offset += sizeof(uint32_t);
		memcpy(stream + offset, instruccion->nombre, instruccion->nombre_length);
		offset += instruccion->nombre_length;

		memcpy(stream + offset, &(instruccion->parametro1_length), sizeof(uint32_t));
		offset += sizeof(uint32_t);
		memcpy(stream + offset, instruccion->parametro1, instruccion->parametro1_length);
		offset += instruccion->parametro1_length;

		memcpy(stream + offset, &(instruccion->parametro2_length), sizeof(uint32_t));
		offset += sizeof(uint32_t);
		memcpy(stream + offset, instruccion->parametro2, instruccion->parametro2_length);
		offset += instruccion->parametro2_length;

		memcpy(stream + offset, &(instruccion->parametro3_length), sizeof(uint32_t));
		offset += sizeof(uint32_t);
		memcpy(stream + offset, instruccion->parametro3, instruccion->parametro3_length);
		offset += instruccion->parametro3_length;
	}

	buffer->stream = stream;

	t_paquete* paquete = malloc(sizeof(t_paquete));


	paquete->codigo_operacion = PAQUETE_INSTRUCCIONES; // Podemos usar una constante por operación

	paquete->buffer = buffer; // Nuestro buffer de antes.

	// Armamos el stream a enviar
	void* a_enviar = malloc(buffer->size + sizeof(uint8_t) + sizeof(uint32_t));
	offset = 0;

	memcpy(a_enviar + offset, &(paquete->codigo_operacion), sizeof(uint8_t));
	offset += sizeof(uint8_t);

	memcpy(a_enviar + offset, &(paquete->buffer->size), sizeof(uint32_t));
	offset += sizeof(uint32_t);

	memcpy(a_enviar + offset, paquete->buffer->stream, paquete->buffer->size);

	ip = config_get_string_value(config, "IP_KERNEL");
	puerto = config_get_string_value(config, "PUERTO_KERNEL");

	log_info(logger, "Lei la IP %s y el PUERTO %s\n", ip, puerto);

	conexion_kernel = crear_conexion_handshake(ip, puerto, CONSOLA);

	if(conexion_kernel == 1){
		free(a_enviar);
		terminar_programa(conexion_kernel, logger, config, f_instrucciones, lista_instrucciones,paquete);
		return SOCKET_ERROR;
	}

	int cantidad_bytes_enviados = send(conexion_kernel, a_enviar, buffer->size + sizeof(uint8_t) + sizeof(uint32_t), 0);

	if (cantidad_bytes_enviados == -1) {
		free(a_enviar);
		terminar_programa(conexion_kernel, logger, config, f_instrucciones, lista_instrucciones,paquete);
		return SEND_ERROR;
	}

	log_info(logger, "paquete enviado, cantidad de bytes enviados: %d", cantidad_bytes_enviados);

	free(a_enviar);



	//t_list* lista_instrucciones_deserializadas = deserializar_lista_instrucciones(paquete);

	//list_iterate(lista_instrucciones_deserializadas, mostrar_instruccion);


	//log_info(logger, "Stream de datos enviados: %s", stream_datos);
	uint8_t respuesta_kernel;

	recv(conexion_kernel, &respuesta_kernel, sizeof(uint8_t), MSG_WAITALL);

	switch (respuesta_kernel){
		case FINALIZA_PROCESO:
			log_info(logger, "Kernel avisó que terminó el proceso por una instrucción EXIT.");
			break;
		case SEGMENTATION_FAULT:
			log_info(logger, "Kernel avisó que ocurrió un segmentation fault.");
			break;
		case MOTIVO_DESCONOCIDO:
			log_info(logger, "Kernel avisó que terminó el proceso por un motivo desconocido.");
			break;
		case SIN_MEMORIA:
			log_info(logger, "Kernel avisó que no hay memoria suficiente para crear un segmento del proceso.");
			break;
		case RECURSO_INVALIDO:
			log_info(logger, "Kernel avisó que se operó sobre un recurso inválido.");
			break;
		default:
			log_info(logger, "Respuesta inesperada. Termina proceso.");
			terminar_programa(conexion_kernel, logger, config, f_instrucciones, lista_instrucciones,paquete);
			return -1;

	}
	terminar_programa(conexion_kernel, logger, config, f_instrucciones, lista_instrucciones,paquete);
	return 0;
}

t_log* iniciar_logger(void)
{
	t_log* nuevo_logger;

	nuevo_logger = log_create("./consola.log", "Consola", 1, LOG_LEVEL_INFO);

	return nuevo_logger;
}

t_config* iniciar_config(char* path_config)
{
	t_config* nuevo_config;

	nuevo_config = config_create(path_config);

	return nuevo_config;
}

void terminar_programa(int conexion, t_log* logger, t_config* config, FILE* f_instrucciones, t_list* lista_instrucciones, t_paquete* paquete)
{
	log_info(logger, "Liberando memoria...");
	if(logger != NULL)
		log_destroy(logger);

	if(config != NULL)
		config_destroy(config);

	close(conexion);

	list_destroy_and_destroy_elements(lista_instrucciones,(void*) instruccion_destroy);

	eliminar_paquete(paquete);

	fclose(f_instrucciones);
}

