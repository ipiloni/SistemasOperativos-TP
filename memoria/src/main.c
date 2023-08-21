#include "main.h"
t_log* logger;
t_config* config;

//semaforos
sem_t semaforoProcesarConexion[3];
sem_t muertePrograma;
sem_t handshakeTerminado;
int handshakesCorrectos;

int main(int argc, char** argv) {

	if (argc < 2)
			return -1;

	config = config_create(argv[1]);
	char* puerto_escucha = config_get_string_value(config, "PUERTO_ESCUCHA");

	inicializarSemaforos();
	logger = iniciar_logger();

	int server_fd = iniciar_servidor(puerto_escucha);
	log_info(logger, "Memoria lista para recibir en puerto: %s \n",puerto_escucha);

	//Manejo de hilos
	//testIfEstructurasFunciona();

	handshakesCorrectos = 0;
	while (handshakesCorrectos < 3) {
		pthread_t thread;
		int* socket_cliente = malloc(sizeof(int));

		*socket_cliente = accept(server_fd, NULL, NULL);
		pthread_create(&thread, NULL, (void*) comunicacion, socket_cliente);
		sem_wait(&handshakeTerminado);

	}

	//Creo estructuras

	inicializarMemoria();
	crearSegmentoCero();

	log_info(logger,"Estructuras inicializadas");

	for (int i = 0; i < 3; i++) { //Les digo a los hilos que pueden arrancar a procesar sus conexiones en paz, estructuras iniciadas
		 sem_post(&semaforoProcesarConexion[i]);
	}

	for (int i = 0; i < 3; i++) {
		sem_destroy(&semaforoProcesarConexion[i]);
	}

	sem_wait(&muertePrograma); // NADIE LE HACE POST
	log_destroy(logger);
	config_destroy(config);

	sem_destroy(&muertePrograma);
	return EXIT_SUCCESS;
}


t_log* iniciar_logger(void)
{
    t_log* nuevo_logger;
    nuevo_logger= log_create("memoria.log", "Servidor", 1, LOG_LEVEL_DEBUG);
    return nuevo_logger;
}

void inicializarSemaforos(void){
	for (int i = 0; i < 3; i++) {
		     sem_init(&semaforoProcesarConexion[i], 0, 0);
		}
	sem_init(&muertePrograma, 0, 0);
	sem_init(&handshakeTerminado, 0, 0);
}

void testIfEstructurasFunciona(){
	/*
	inicializarMemoria();
	crearSegmentoCero();
	t_paquete* paquete = crear_paquete();
	crearProceso(1);
	crearProceso(2);
	eliminarProceso(10);
	cargarPaqueteCrearSegmento(paquete,puedoCrearSegmento(1, 1, 3700),1,1,3700);
	cargarPaqueteCrearSegmento(paquete,puedoCrearSegmento(1, 2, 100),1,2,100);
	eliminarSegmento(1,10);
	eliminarSegmento(1,1);
	cargarPaqueteCrearSegmento(paquete,puedoCrearSegmento(1, 1, 3750),1,1,3750);
	cargarPaqueteCrearSegmento(paquete,puedoCrearSegmento(1, 1, 25),1,1,25);
	compactarSegmentos();
	cargarPaqueteCrearSegmento(paquete,puedoCrearSegmento(1, 1, 3750),1,1,3750);
	eliminar_paquete(paquete);

	escribirEspacioUsuario("Hola como estas?",1,50,17,CPU);
	char * tuvieja = malloc(17);
	leerEspacioUsuario(tuvieja,1,50,17,CPU);
	log_info(logger, "Lo que lei fue %s", tuvieja);
	free(tuvieja);
	*/
}
