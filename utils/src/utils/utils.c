#include "utils.h"

char* obtener_motivo(cod_instruccion motivo) {
	switch(motivo) {
		case SET: return "IO"; break;
		case MOV_IN: return "F_OPEN"; break;
		case MOV_OUT: return "F_CLOSE"; break;
		case IO: return "IO"; break;
		case F_OPEN: return "F_OPEN"; break;
		case F_CLOSE: return "F_CLOSE"; break;
		case F_SEEK: return "F_SEEK"; break;
		case F_READ: return "F_READ"; break;
		case F_TRUNCATE: return "F_TRUNCATE"; break;
		case F_WRITE: return "F_WRITE"; break;
		case WAIT: return "WAIT"; break;
		case SIGNAL: return "SIGNAL"; break;
		case CREATE_SEGMENT: return "CREATE_SEGMENT"; break;
		case DELETE_SEGMENT: return "DELETE_SEGMENT"; break;
		case YIELD: return "YIELD"; break;
		case EXIT: return "EXIT"; break;
		case SEG_FAULT: return "SEGMENTATION_FAULT"; break;
		default: return "";
	}
}

char* obtener_modulo(handshake modulo) {
	switch(modulo) {
		case KERNEL: return "KERNEL"; break;
		case CPU: return "CPU"; break;
		case FILESYSTEM: return "FILESYSTEM"; break;
		case MEMORIA: return "MEMORIA"; break;
		case CONSOLA: return "CONSOLA"; break;
		default: return "";
	}
}

char* obtener_cod_op(op_code codigo) {
	switch(codigo) {
		case MENSAJE: return "MENSAJE"; break;
		case PAQUETE: return "PAQUETE"; break;
		case PAQUETE_INSTRUCCIONES: return "PAQUETE_INSTRUCCIONES"; break;
		case INSTRUCCION: return "INSTRUCCION"; break;
		case CONTEXTO_DE_EJECUCION: return "CONTEXTO_DE_EJECUCION"; break;
		case TABLA_SEGMENTOS: return "TABLA_SEGMENTOS"; break;
		case CREAR_PROCESO: return "CREAR_PROCESO"; break;
		case ELIMINAR_PROCESO: return "ELIMINAR_PROCESO"; break;
		case ELIMINAR_SEGMENTO: return "ELIMINAR_SEGMENTO"; break;
		case COMPACTAR: return "COMPACTAR"; break;
		case ESCRIBIR_ESPACIO_USUARIO: return "ESCRIBIR_ESPACIO_USUARIO"; break;
		case LEER_ESPACIO_USUARIO: return "LEER_ESPACIO_USUARIO"; break;
		case CREAR_SEGMENTO: return "CREAR_SEGMENTO"; break;
		case OK: return "OK"; break;
		case OUT_OF_MEMORY: return "OUT_OF_MEMORY"; break;
		case ID_YA_CREADA: return "ID_YA_CREADA"; break;
		case LIMITE_TABLA_SEGMENTOS: return "LIMITE_TABLA_SEGMENTOS"; break;
		case NECESITA_COMPACTACION: return "NECESITA_COMPACTACION"; break;
		case ABRIR_ARCHIVO: return "ABRIR_ARCHIVO"; break;
		case CREAR_ARCHIVO: return "CREAR_ARCHIVO"; break;
		case CERRAR_ARCHIVO: return "CERRAR_ARCHIVO"; break;
		case RECORTAR_ARCHIVO: return "RECORTAR_ARCHIVO"; break;
		case ESCRIBIR_ARCHIVO: return "ESCRIBIR_ARCHIVO"; break;
		case LEER_ARCHIVO: return "LEER_ARCHIVO"; break;
		case NO_EXISTE_ARCHIVO: return "NO_EXISTE_ARCHIVO"; break;
		case ARCHIVO_ABIERTO: return "ARCHIVO_ABIERTO"; break;
		default: return "xd";
	}
}

cod_instruccion obtener_enum_de_instruccion(char * string_instruccion) {
	const char *instrucciones_posibles[] = {
			"SET",
			"MOV_IN",
			"MOV_OUT",
			"I/O",
			"F_OPEN",
			"F_CLOSE",
			"F_SEEK",
			"F_READ",
			"F_TRUNCATE",
			"F_WRITE",
			"WAIT",
			"SIGNAL",
			"CREATE_SEGMENT",
			"DELETE_SEGMENT",
			"YIELD",
			"EXIT",
	        NULL //valor centinela
	  };

	  for (int i = 0; instrucciones_posibles[i] != NULL; i++) {
		  if (strcmp(string_instruccion, instrucciones_posibles[i]) == 0) {
			  return (cod_instruccion)i;
	       }

	  }

	  log_error(logger, "No encontre una instruccion con ese nombre XDDDDD");
	  sleep(10);
	  return -1; // valor default
}

void inicializar_registros_cpu(t_registros_cpu* registro) {
	memset(registro->AX, '0', sizeof(registro->AX));
	memset(registro->BX, '0', sizeof(registro->BX));
	memset(registro->CX, '0', sizeof(registro->CX));
	memset(registro->DX, '0', sizeof(registro->DX));

	memset(registro->EAX, '0', sizeof(registro->EAX));
	memset(registro->EBX, '0', sizeof(registro->EBX));
	memset(registro->ECX, '0', sizeof(registro->ECX));
	memset(registro->EDX, '0', sizeof(registro->EDX));

	memset(registro->RAX, '0', sizeof(registro->RAX));
	memset(registro->RBX, '0', sizeof(registro->RBX));
	memset(registro->RCX, '0', sizeof(registro->RCX));
	memset(registro->RDX, '0', sizeof(registro->RDX));
}

t_instruccion* deserializar_instruccion(t_buffer* buffer) {
    t_instruccion* instruccion = malloc(sizeof(t_instruccion));

    void* stream = buffer->stream;

    memcpy(&(instruccion->nombre_length), stream, sizeof(uint32_t));
    stream += sizeof(uint32_t);
    instruccion->nombre = malloc(instruccion->nombre_length);
    memcpy(instruccion->nombre, stream, instruccion->nombre_length);
    stream += instruccion->nombre_length;

    memcpy(&(instruccion->parametro1_length), stream, sizeof(uint32_t));
	stream += sizeof(uint32_t);
	instruccion->parametro1 = malloc(instruccion->parametro1_length);
	memcpy(instruccion->parametro1, stream, instruccion->parametro1_length);
	stream += instruccion->parametro1_length;

    memcpy(&(instruccion->parametro2_length), stream, sizeof(uint32_t));
	stream += sizeof(uint32_t);
	instruccion->parametro2 = malloc(instruccion->parametro2_length);
	memcpy(instruccion->parametro2, stream, instruccion->parametro2_length);
	stream += instruccion->parametro2_length;

    memcpy(&(instruccion->parametro3_length), stream, sizeof(uint32_t));
	stream += sizeof(uint32_t);
	instruccion->parametro3 = malloc(instruccion->parametro3_length);
	memcpy(instruccion->parametro3, stream, instruccion->parametro3_length);
	stream += instruccion->parametro3_length;

	buffer->stream = stream;
    return instruccion;
}

t_segmento * deserializar_segmento(t_buffer* buffer) {
    t_segmento* segmento = malloc(sizeof(t_segmento));

    void* stream = buffer->stream;

    memcpy(&(segmento->pid), stream, sizeof(uint32_t));
    stream += sizeof(uint32_t);

    memcpy(&(segmento->idSegmento), stream, sizeof(uint32_t));
	stream += sizeof(uint32_t);

    memcpy(&(segmento->base), stream, sizeof(uint32_t));
	stream += sizeof(uint32_t);

    memcpy(&(segmento->tamanio), stream, sizeof(uint32_t));
	stream += sizeof(uint32_t);

	buffer->stream = stream;
    return segmento;
}

void* serializar_paquete(t_paquete* paquete, int bytes)
{
	void * a_enviar = malloc(bytes);
	int desplazamiento = 0;

	memcpy(a_enviar + desplazamiento, &(paquete->codigo_operacion), sizeof(uint8_t));
	desplazamiento+= sizeof(uint8_t);

	memcpy(a_enviar + desplazamiento, &(paquete->buffer->size), sizeof(int));
	desplazamiento+= sizeof(int);

	memcpy(a_enviar + desplazamiento, paquete->buffer->stream, paquete->buffer->size);
	desplazamiento+= paquete->buffer->size;

	log_debug(logger, "Enviar paquete - Codigo de Operacion: %s - Cantidad de bytes: %d", obtener_cod_op(paquete->codigo_operacion), paquete->buffer->size);
	return a_enviar;
}

int crear_conexion(char *ip, char* puerto)
{
	struct addrinfo hints;
	struct addrinfo *server_info;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(ip, puerto, &hints, &server_info);

	// Ahora vamos a crear el socket.
	int socket_cliente = socket(server_info->ai_family,
            server_info->ai_socktype,
            server_info->ai_protocol);

	// Ahora que tenemos el socket, vamos a conectarlo
	connect(socket_cliente, server_info->ai_addr, server_info->ai_addrlen);

	freeaddrinfo(server_info);

	return socket_cliente;
}

int crear_conexion_handshake(char *ip, char* puerto, int handshake)
{
	struct addrinfo hints;
	struct addrinfo *servinfo;
	struct addrinfo *p;
	int socket_cliente;
	int status;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if((status = getaddrinfo(ip, puerto, &hints, &servinfo)) != 0) {
			fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
			exit(-1);
		}
	for(p = servinfo; p != NULL; p = p->ai_next) {
	        if ((socket_cliente = socket(p->ai_family, p->ai_socktype,
	                p->ai_protocol)) == -1) {
	            perror("client: socket");
	            continue;
	        }

	        if (connect(socket_cliente, p->ai_addr, p->ai_addrlen) == -1) {
	            close(socket_cliente);
	            perror("client: connect");
	            continue;
	        }

	        break;
	    }

	if (p == NULL) {
	        fprintf(stderr, "client: failed to connect\n");
	        freeaddrinfo(servinfo);
	        return -1;
	    }

	freeaddrinfo(servinfo);

	uint32_t result;

	if (send(socket_cliente, &handshake, sizeof(uint32_t), 0) == -1) {
		perror("send");
		close(socket_cliente);
		exit(-1);
	}

	if (recv(socket_cliente, &result, sizeof(uint32_t), MSG_WAITALL) == -1) {
		perror("recv");
		close(socket_cliente);
		exit(-1);
	}

	if(result == -1) {
		perror("Handshake fallido.");
		return -1;
	}

	log_info(logger,"Handshake con el puerto %s conseguido",puerto);
	return socket_cliente;
}

void instruccion_destroy(t_instruccion* instruccion){
	free(instruccion->nombre);
	free(instruccion->parametro1);
	free(instruccion->parametro2);
	free(instruccion->parametro3);
	free(instruccion);
}

void enviar_mensaje(char* mensaje, int socket_cliente)
{
	t_paquete* paquete = malloc(sizeof(t_paquete));

	paquete->codigo_operacion = MENSAJE;
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = strlen(mensaje) + 1;
	paquete->buffer->stream = malloc(paquete->buffer->size);
	memcpy(paquete->buffer->stream, mensaje, paquete->buffer->size);

	int bytes = paquete->buffer->size + 2*sizeof(int);

	void* a_enviar = serializar_paquete(paquete, bytes);

	send(socket_cliente, a_enviar, bytes, 0);

	free(a_enviar);
	eliminar_paquete(paquete);
}


void crear_buffer(t_paquete* paquete)
{
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = 0;
	paquete->buffer->stream = NULL;
}

t_paquete* crear_paquete(void)
{
	t_paquete* paquete = malloc(sizeof(t_paquete));
	paquete->codigo_operacion = 0;
	crear_buffer(paquete);
	return paquete;
}

void agregar_a_paquete(t_paquete* paquete, void* valor, int tamanio)
{
	paquete->buffer->stream = realloc(paquete->buffer->stream, paquete->buffer->size + tamanio + sizeof(int));

	memcpy(paquete->buffer->stream + paquete->buffer->size, &tamanio, sizeof(int));
	memcpy(paquete->buffer->stream + paquete->buffer->size + sizeof(int), valor, tamanio);

	paquete->buffer->size += tamanio + sizeof(int);
}

void enviar_paquete(t_paquete* paquete, int socket_cliente)
{
	int bytes = paquete->buffer->size + sizeof(int) + sizeof(uint8_t); // Tam del tam + cod op + tamaño del buffer
	void* a_enviar = serializar_paquete(paquete, bytes);

	send(socket_cliente, a_enviar, bytes, 0); // syscall param socketcall.sento(msg) points to uninitialised byte(s)

	free(a_enviar);
}

void eliminar_paquete(t_paquete* paquete)
{
	free(paquete->buffer->stream);
	free(paquete->buffer);
	free(paquete);
}


void liberar_conexion(int socket_cliente) {
	close(socket_cliente);
}

//SERVIDOR ---------------------

int iniciar_servidor(char* puerto_escucha)
{

	int socket_servidor;
	int status;
	int yes = 1;

	struct addrinfo hints, *servinfo, *p;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	if((status = getaddrinfo(NULL, puerto_escucha, &hints, &servinfo)) != 0) { // servidor local independiente de direccion ip
			fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
			exit(1);
		}
		for (p=servinfo; p != NULL; p = p->ai_next) { // itero la lista de addrinfo para tener opciones de respaldo en caso de fallo
		      if ((socket_servidor = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {// si falla socket, continua iterando
		    	  perror("server: socket");
		    	  continue;
		      }
			  if (setsockopt(socket_servidor, SOL_SOCKET, SO_REUSEADDR, &yes,
							  sizeof(int)) == -1) {
						  perror("setsockopt");
						  exit(1);
					  }
		      if (bind(socket_servidor, p->ai_addr, p->ai_addrlen) == -1) { // si falla bind, cierra el socket y continua iterando
		          close(socket_servidor);
		          perror("server: bind");
		          continue;
		      }
		      break; // se consiguió bindear un socket
		  }

	freeaddrinfo(servinfo);

	if (p == NULL)  {
		fprintf(stderr, "server: failed to bind\n");
		exit(1);
	}

	if(listen(socket_servidor, SOMAXCONN) == -1) {
		perror("listen");
		exit(1);
	}

	log_trace(logger, "Listo para escuchar a mi cliente");

	return socket_servidor;
}

int esperar_cliente(int socket_servidor)
{
	int socket_cliente = accept(socket_servidor, NULL, NULL);

	log_info(logger, "Se conecto un cliente!");

	return socket_cliente;
}


int recibir_operacion(int socket_cliente) // NO FUNCIONA cambiar a op_code
{
	int cod_op;
	if(recv(socket_cliente, &cod_op, sizeof(uint8_t), MSG_WAITALL) > 0) {
		return cod_op;
	}
	else
	{
		close(socket_cliente);
		return -1;
	}
}

void* recibir_buffer(int* size, int socket_cliente)
{
	void * buffer;

	recv(socket_cliente, size, sizeof(u_int32_t), MSG_WAITALL);
	buffer = malloc(*size);
	recv(socket_cliente, buffer, *size, MSG_WAITALL);

	return buffer;
}

t_paquete* recibir_paquete(t_paquete* paquete,int socket){

	int bytes_recibidos = recv(socket, &(paquete->codigo_operacion), sizeof(uint8_t), MSG_WAITALL);

	if(bytes_recibidos == 0){
		paquete->codigo_operacion = ERROR;
		log_error(logger,"Recibio 0 bytes. Probablemente se desconectó el cliente");
	}else if(bytes_recibidos == -1){
		paquete->codigo_operacion = ERROR;
		log_error(logger,"Error al recibir un paquete");
	}

	recv(socket, &(paquete->buffer->size), sizeof(int), 0);

	paquete->buffer->stream = malloc(paquete->buffer->size);

	recv(socket, paquete->buffer->stream, paquete->buffer->size, 0);

	log_debug(logger,"Paquete recibido -> Codigo: %s Tamaño del buffer: %d", obtener_cod_op(paquete->codigo_operacion),paquete->buffer->size);
	return paquete;
}

void imprimirSegmentos(t_list* lista) {
	log_debug(logger,"Los segmentos actuales de esta lista son los siguientes\n");
	list_iterate(lista, (void*) iteradorPrintTablaSegmentos);
}

void iteradorPrintTablaSegmentos(t_segmento* segmento) {
    log_debug(logger,"PID %d - ID %d - Base %d - Tamaño %d\n",segmento->pid, segmento-> idSegmento, segmento->base, segmento->tamanio);
}

t_list* recibir_tabla_segmentos(int socket_memoria){
	t_paquete* paquete = crear_paquete();
	paquete = recibir_paquete(paquete, socket_memoria);

	t_list* lista_segmentos;
	if(paquete->codigo_operacion != TABLA_SEGMENTOS) {
		log_debug(logger, "Operación desconocida.\n");
		return NULL;
		}
	lista_segmentos = deserializar_lista_segmentos(paquete->buffer->stream,paquete->buffer->size);

	eliminar_paquete(paquete);
	return lista_segmentos;
}


t_list* deserializar_lista_segmentos(void* serializedData, int serializedSize) {
    t_list* listaSegmentos = list_create();

    int numSegments = serializedSize / (sizeof(uint32_t) * 4);

    for (int i = 0; i < numSegments; i++) {
        t_segmento* segmento = malloc(sizeof(t_segmento));

        memcpy(&segmento->pid, serializedData + i * (sizeof(uint32_t) * 4), sizeof(uint32_t));
        memcpy(&segmento->idSegmento, serializedData + i * (sizeof(uint32_t) * 4) + sizeof(uint32_t), sizeof(uint32_t));
        memcpy(&segmento->base, serializedData + i * (sizeof(uint32_t) * 4) + sizeof(uint32_t) * 2, sizeof(uint32_t));
        memcpy(&segmento->tamanio, serializedData + i * (sizeof(uint32_t) * 4) + sizeof(uint32_t) * 3, sizeof(uint32_t));

        list_add(listaSegmentos, segmento);
    }

    return listaSegmentos;
}

void* serializar_lista_segmentos(t_list* listaSegmentos, int* serializedSize) {

	int numSegments = list_size(listaSegmentos);
    int segmentSize = sizeof(uint32_t) * 4;
    *serializedSize = numSegments * segmentSize;

    void* serializedData = malloc(*serializedSize);

    for (int i = 0; i < numSegments; i++) {
        t_segmento* segmento = list_get(listaSegmentos, i);

        memcpy(serializedData + i * segmentSize, &segmento->pid, sizeof(uint32_t));
        memcpy(serializedData + i * segmentSize + sizeof(uint32_t), &segmento->idSegmento, sizeof(uint32_t));
        memcpy(serializedData + i * segmentSize + sizeof(uint32_t) * 2, &segmento->base, sizeof(uint32_t));
        memcpy(serializedData + i * segmentSize + sizeof(uint32_t) * 3, &segmento->tamanio, sizeof(uint32_t));
    }

    return serializedData;
}

t_segmento *encontrarSegmentoPorIDYTabla(int idSegmento,t_list * tablaSegmentos) {
	int esElSegmentoPorID(t_segmento *s) {
	       if (s->idSegmento == idSegmento){
	          return true;
	       } else {
	    	   return false;
	       }
	 }
    return list_find(tablaSegmentos, (void*) esElSegmentoPorID);
}

char* solicitar_a_memoria(uint32_t direccion_fisica, int socket_memoria, uint32_t bytes, uint32_t pid) {
	t_paquete* paquete = crear_paquete();
	paquete->codigo_operacion = LEER_ESPACIO_USUARIO;
	paquete->buffer->size = sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint32_t); // direccion + bytes + pid
	paquete->buffer->stream = malloc(paquete->buffer->size);

	int offset = 0;
	memcpy(paquete->buffer->stream + offset, &(pid), sizeof(uint32_t));
	offset += sizeof(uint32_t);

	memcpy(paquete->buffer->stream + offset, &(direccion_fisica), sizeof(uint32_t));
	offset += sizeof(uint32_t);

	memcpy(paquete->buffer->stream + offset, &(bytes), sizeof(uint32_t));

	enviar_paquete(paquete, socket_memoria);

	eliminar_paquete(paquete);

	return recibir_resultado_memoria(socket_memoria, bytes);
}

char* recibir_resultado_memoria(int socket_memoria, uint32_t bytes) {
	t_paquete* paquete = crear_paquete();

	paquete = recibir_paquete(paquete,socket_memoria);

	if(paquete->codigo_operacion == OK){

		char* valor = malloc(bytes+1);

		memcpy(valor, paquete->buffer->stream, bytes);
		valor[bytes] = '\0';

		eliminar_paquete(paquete);

		return valor;
	}
	else {
		log_error(logger,"No hemos recibido el resultado correctamente");
		return "";
	}
}

op_code enviar_a_memoria(int socket_memoria, uint32_t direccion_fisica, char* valor, uint32_t pid, uint32_t bytes) {
	t_paquete* paquete = crear_paquete();
	paquete->codigo_operacion = ESCRIBIR_ESPACIO_USUARIO;
	paquete->buffer->size = sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint32_t)  + bytes;
	paquete->buffer->stream = malloc(paquete->buffer->size);
	//pid, direccion, bytes,valor

	int offset = 0;
	memcpy(paquete->buffer->stream + offset, &(pid), sizeof(uint32_t));
	offset += sizeof(uint32_t);

	memcpy(paquete->buffer->stream + offset, &(direccion_fisica), sizeof(uint32_t));
	offset += sizeof(uint32_t);

	memcpy(paquete->buffer->stream + offset, &(bytes), sizeof(uint32_t));
	offset += sizeof(uint32_t);

	memcpy(paquete->buffer->stream + offset, valor, bytes);

	enviar_paquete(paquete, socket_memoria);

	eliminar_paquete(paquete);

	op_code respuesta;

	if(recv(socket_memoria, &(respuesta), sizeof(uint32_t), MSG_WAITALL)==0){
		return -1;
	}

	return respuesta;
}

char* itoa(int numero) {

    int numDigitos = snprintf(NULL, 0, "%d", numero);

    char* str = (char*)malloc((numDigitos + 1));

    snprintf(str, numDigitos + 1, "%d", numero);

    return str;
}

int ceiling(double x) { // NO LA HIZO CHATGPT
    int intPart = (int)x;  // Get the integer part of the number

    if (x > intPart) {
        return intPart + 1;  // If x has a fractional part, round up by adding 1 to the integer part
    }
    else {
        return intPart;  // If x is already an integer, return the integer part as it is
    }
}


