#include "main.h"

t_log* logger;
t_config *config;
t_bitarray* bitmap_bloques;
void * bitarray;
size_t tamanioBitArray;
int tamanioBloque;
char* pathArchBloques;

int main(int argc, char** argv) {
	if (argc < 2)
			return -1;

	int cantidadBloques;
	t_config *superBloque;

	char *path_superbloque, *path_FCB, *puerto_escucha;

    t_conexion_info_memoria* info_memoria = malloc(sizeof(t_conexion_info_memoria));

    logger = iniciar_logger();
	config = config_create(argv[1]);

	//Esto saca del fileSystem.config datos y se los encaja a los punteros
	extraerDeConfig(&info_memoria->path_bitmap, &path_FCB, &pathArchBloques, &path_superbloque,&info_memoria->ip,&info_memoria->puerto_memoria,&puerto_escucha);

    //se levanta el superBloque que esta en la carpeta dat
	superBloque = config_create(path_superbloque);
	levantarSuperbloque(superBloque,&tamanioBloque,&cantidadBloques);

	tamanioBitArray = cantidadBloques/8; // dividido 8 son bytes

	// Me conecto a Memoria
	int socket_memoria = conectar_a_memoria(info_memoria);
	levantar_bitmap(info_memoria);

	int socket_server = iniciar_servidor(puerto_escucha);
	int socket_cliente = accept(socket_server, NULL, NULL);

	log_info(logger, "Filesystem listo para recibir en puerto: %s\n",puerto_escucha);

	uint32_t handshake;
	uint32_t resultOk = 0;
	uint32_t resultError = -1;

	recv(socket_cliente, &handshake, sizeof(uint32_t), MSG_WAITALL);
	if(handshake == KERNEL){
		send(socket_cliente, &resultOk, sizeof(uint32_t), 0);
		procesar_conexion(socket_cliente, socket_memoria);

	} else {
		send(socket_cliente, &resultError, sizeof(uint32_t), 0);
	}

	log_info(logger,"Termina FileSystem");


	free(info_memoria);
	close(socket_cliente);
	config_destroy(config);
	config_destroy(superBloque);
	return EXIT_SUCCESS;
}

void procesar_conexion(int socket_kernel, int socket_memoria) {

	while(1) {

		t_paquete* paquete = crear_paquete();

		paquete = recibir_paquete(paquete, socket_kernel);

		if(paquete->codigo_operacion == ERROR){
			eliminar_paquete(paquete);
			return;
		}

		void * stream_original = paquete->buffer->stream;

		char* nombreArchivo = recibir_nombre_de_kernel(paquete->buffer); // Todas las op necesitan el nombre del archivo

		switch (paquete->codigo_operacion) {
		case CREAR_ARCHIVO:
			crear_archivo(nombreArchivo, socket_kernel);
			break;
		case ABRIR_ARCHIVO:
			abrir_archivo(nombreArchivo, socket_kernel);
		    break;
		case LEER_ARCHIVO:
			{
			uint32_t dir_fisica;
			memcpy(&dir_fisica, paquete->buffer->stream, sizeof(uint32_t));
			paquete->buffer->stream += sizeof(uint32_t);
			uint32_t tamanioTotalAcceso;
			memcpy(&tamanioTotalAcceso, paquete->buffer->stream, sizeof(uint32_t));
			paquete->buffer->stream += sizeof(uint32_t);
			uint32_t pid;
			memcpy(&pid, paquete->buffer->stream, sizeof(uint32_t));
			paquete->buffer->stream += sizeof(uint32_t);
			uint32_t punteroArchivo;
			memcpy(&punteroArchivo, paquete->buffer->stream, sizeof(uint32_t));
			paquete->buffer->stream += sizeof(uint32_t);

			log_debug(logger, "Leyendo archivo... %s", nombreArchivo);
			acceder_archivo(nombreArchivo,punteroArchivo, dir_fisica ,tamanioTotalAcceso, pid ,LEER_ARCHIVO, socket_memoria);

			enviar_mensaje_a_kernel(socket_kernel, OK);
			}
			break;
		case ESCRIBIR_ARCHIVO:
			{

			uint32_t dir_fisica;
			memcpy(&dir_fisica, paquete->buffer->stream, sizeof(uint32_t));
			paquete->buffer->stream += sizeof(uint32_t);
			uint32_t tamanioTotalAcceso;
			memcpy(&tamanioTotalAcceso, paquete->buffer->stream, sizeof(uint32_t));
			paquete->buffer->stream += sizeof(uint32_t);
			uint32_t pid;
			memcpy(&pid, paquete->buffer->stream, sizeof(uint32_t));
			paquete->buffer->stream += sizeof(uint32_t);
			uint32_t punteroArchivo;
			memcpy(&punteroArchivo, paquete->buffer->stream, sizeof(uint32_t));
			paquete->buffer->stream += sizeof(uint32_t);

			log_debug(logger, "Escribiendo Archivo... %s.", nombreArchivo);
			acceder_archivo(nombreArchivo,punteroArchivo, dir_fisica ,tamanioTotalAcceso, pid ,ESCRIBIR_ARCHIVO, socket_memoria);
			
			enviar_mensaje_a_kernel(socket_kernel, OK);

			}
			break;
		case RECORTAR_ARCHIVO:
			log_debug(logger, "Truncando Archivo... %s", nombreArchivo);

			uint32_t nuevo_tamanio;
			memcpy(&nuevo_tamanio, paquete->buffer->stream, sizeof(uint32_t));
			paquete->buffer->stream += sizeof(uint32_t);

			truncar_archivo(nombreArchivo, nuevo_tamanio);

			enviar_mensaje_a_kernel(socket_kernel, OK);

			break;
		default:
			log_warning(logger, "Operacion desconocida");
			break;
	 }

	 free(nombreArchivo);
	 paquete->buffer->stream = stream_original;
	 eliminar_paquete(paquete);
  }
 return;
}

int conectar_a_memoria(t_conexion_info_memoria* info_memoria){

    int socket_a_Memoria = crear_conexion(info_memoria->ip, info_memoria->puerto_memoria);

	//Declaracion para hacer el handshake con memoria
    uint32_t handshakeCliente = FILESYSTEM; //Hola soy FileSystem
	uint32_t resultConexion;

	send(socket_a_Memoria, &handshakeCliente, sizeof(uint32_t), 0); // Le mando mis saludos a Memoria
	recv(socket_a_Memoria, &resultConexion, sizeof(uint32_t), MSG_WAITALL);

    //Memoria respondio a mi saludo, veo que hago.
	if(resultConexion == 0){
	   log_info(logger, "Conexion con memoria exitosa.");
	}

	return socket_a_Memoria;
}
