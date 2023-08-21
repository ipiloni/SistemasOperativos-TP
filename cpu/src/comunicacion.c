#include "comunicacion.h"

int comunicarse_a_kernel(int socket_kernel, int socket_memoria) {
	t_paquete* paquete = crear_paquete();

	if(recv(socket_kernel, &(paquete->codigo_operacion), sizeof(uint8_t), MSG_WAITALL)==0){
		log_info(logger, "Se ha desconectado el kernel ");
		return 1;
	}

	switch(paquete->codigo_operacion) {
		case CONTEXTO_DE_EJECUCION:
			recv(socket_kernel, &(paquete->buffer->size), sizeof(uint32_t), 0);
			paquete->buffer->stream = malloc(paquete->buffer->size);
			void* stream_original = paquete->buffer->stream;
			contexto = recibir_contexto(socket_kernel, paquete->buffer);
			paquete->buffer->stream = stream_original;
			eliminar_paquete(paquete);
			int cantidad_instrucciones = contexto->cant_instrucciones;
			se_necesita_cambio_de_contexto = 0;
			while((contexto->program_counter < cantidad_instrucciones) && (se_necesita_cambio_de_contexto == 0)) {
				t_instruccion* instruccion = fetch_instruccion(contexto);
				decode_instruccion(instruccion, socket_kernel, socket_memoria);
			}
			eliminar_contexto(contexto);
			break;
		default:
			log_error(logger, "Operacion desconocida.");
			break;
	}
	return 0;
}

void enviar_direccion_fisica(int socket_kernel, uint32_t direccion) {
	if (send(socket_kernel, &direccion, sizeof(uint32_t), 0) == -1) {
		log_error(logger, "Fallo al enviarle la dirección física a Kernel.");
	}
}

int esperar_kernel(int server_cpu) {
	while(1) {
		int socket_kernel = accept(server_cpu, NULL, NULL);

		/* Handshake Kernel */
		uint32_t handshake_kernel;
		uint32_t resultOk = 1;
		uint32_t resultError = -1;

		/* Chequea que la comunicacion sea de Kernel y no de otra cosa */
		recv(socket_kernel, &handshake_kernel, sizeof(uint32_t), MSG_WAITALL);
		if(handshake_kernel == KERNEL) {
			send(socket_kernel, &resultOk, sizeof(uint32_t), 0);
			log_info(logger, "Recibi a Kernel como cliente");
			return socket_kernel;
		}
		else {
			send(socket_kernel, &resultError, sizeof(uint32_t), 0);
			log_info(logger, "Recibi a un cliente desconocido");
		}
	/* Fin Handshake */
	}
}

int conectarse_a_memoria(){
	int socket;
	char* ip_memoria;
	char* puerto_memoria;
	ip_memoria = config_get_string_value(config, "IP_MEMORIA");
	puerto_memoria = config_get_string_value(config, "PUERTO_MEMORIA");

	socket = crear_conexion(ip_memoria, puerto_memoria);

	/* HANDSHAKE  */
	uint32_t handshake_memoria = CPU;
	uint32_t result_memoria = -1;

	send(socket, &handshake_memoria, sizeof(uint32_t), 0);
	recv(socket, &result_memoria, sizeof(uint32_t), MSG_WAITALL);

	if(result_memoria == MEMORIA) {
		log_info(logger, "Conexion a Memoria establecida.");
	} else {
		log_error(logger, "La conexion con Memoria no es correcta.");
		liberar_conexion(socket);
	}

	return socket;
}

t_contexto* recibir_contexto(int socket_cliente, t_buffer* buffer) {
	t_contexto* nuevo_contexto = malloc(sizeof(t_contexto));
	inicializar_contexto(nuevo_contexto);

	recv(socket_cliente, buffer->stream, buffer->size, 0);

	/* Obtengo PID */
	uint32_t pid;
	memcpy(&pid, buffer->stream, sizeof(uint32_t));
	buffer->stream += sizeof(uint32_t);
	nuevo_contexto->pid = pid;

	/* Obtengo Instrucciones */
	uint32_t cantidad_instrucciones = 0;
	memcpy(&cantidad_instrucciones, buffer->stream, sizeof(uint32_t));
	buffer->stream += sizeof(uint32_t);
	nuevo_contexto->cant_instrucciones = cantidad_instrucciones;

	for(int i = 0; i < cantidad_instrucciones; i++) {
		t_instruccion* instruccion = deserializar_instruccion(buffer);
		list_add(nuevo_contexto->instrucciones, instruccion);
	}

	/* Obtengo Program Counter */
	uint32_t pc;
	memcpy(&pc, buffer->stream, sizeof(uint32_t));
	buffer->stream += sizeof(uint32_t);
	nuevo_contexto->program_counter = pc;

	/* Obtengo Registros CPU */
	memcpy(nuevo_contexto->registros_cpu->AX, buffer->stream, 4);
	buffer->stream += 4;
	memcpy(nuevo_contexto->registros_cpu->BX, buffer->stream, 4);
	buffer->stream += 4;
	memcpy(nuevo_contexto->registros_cpu->CX, buffer->stream, 4);
	buffer->stream += 4;
	memcpy(nuevo_contexto->registros_cpu->DX, buffer->stream, 4);
	buffer->stream += 4;
	memcpy(nuevo_contexto->registros_cpu->EAX, buffer->stream, 8);
	buffer->stream += 8;
	memcpy(nuevo_contexto->registros_cpu->EBX, buffer->stream, 8);
	buffer->stream += 8;
	memcpy(nuevo_contexto->registros_cpu->ECX, buffer->stream, 8);
	buffer->stream += 8;
	memcpy(nuevo_contexto->registros_cpu->EDX, buffer->stream, 8);
	buffer->stream += 8;
	memcpy(nuevo_contexto->registros_cpu->RAX, buffer->stream, 16);
	buffer->stream += 16;
	memcpy(nuevo_contexto->registros_cpu->RBX, buffer->stream, 16);
	buffer->stream += 16;
	memcpy(nuevo_contexto->registros_cpu->RCX, buffer->stream, 16);
	buffer->stream += 16;
	memcpy(nuevo_contexto->registros_cpu->RDX, buffer->stream, 16);
	buffer->stream += 16;

	/* Obtengo Tabla de Segmentos */
	uint32_t cantidad_segmentos = 0;
	memcpy(&cantidad_segmentos, buffer->stream, sizeof(uint32_t));
	buffer->stream += sizeof(uint32_t);
	nuevo_contexto->cant_segmentos = cantidad_segmentos;

	for(int i = 0; i < cantidad_segmentos; i++) {
		t_segmento* segmento = deserializar_segmento(buffer);
		list_add(nuevo_contexto->tabla_de_segmentos, segmento);
	}

	return nuevo_contexto;
}

void devolver_contexto_con_dir_fisica(int socket_cliente, cod_instruccion motivo_desalojo, uint32_t direccion) {
	log_info(logger, "Enviando contexto a Kernel con motivo de desalojo: %d", motivo_desalojo);

	t_paquete* paquete = crear_paquete();
	paquete->codigo_operacion = CONTEXTO_DE_EJECUCION;
	paquete->buffer->size = 4*4 + 4*8 + 4*16 + sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint8_t);
							// registros_cpu,  program_counter,   direccion fisica,  motivo_desalojo

	serializar_contexto_parcial(paquete->buffer, motivo_desalojo);

	memcpy(&(paquete->buffer->stream), &direccion , sizeof(uint32_t));

	void* a_enviar = malloc(paquete->buffer->size + sizeof(uint8_t) + sizeof(uint32_t));
	int offset = 0;

	memcpy(a_enviar + offset, &(paquete->codigo_operacion), sizeof(uint8_t));
	offset += sizeof(uint8_t);

	memcpy(a_enviar + offset, &(paquete->buffer->size), sizeof(uint32_t));
	offset += sizeof(uint32_t);

	memcpy(a_enviar + offset, paquete->buffer->stream, paquete->buffer->size);

	send(socket_cliente, a_enviar, paquete->buffer->size + sizeof(uint8_t) + sizeof(uint32_t), 0);
	free(a_enviar);
}

// solo devuelve registros, pc y motivo
void devolver_contexto(int socket_cliente, cod_instruccion motivo_desalojo) {
	log_debug(logger, "Enviando contexto a Kernel con motivo de desalojo: %s", obtener_motivo(motivo_desalojo));

	t_paquete* paquete = crear_paquete();
	paquete->codigo_operacion = CONTEXTO_DE_EJECUCION;
	paquete->buffer->size = 4*4 + 4*8 + 4*16 + sizeof(uint32_t) + sizeof(uint8_t);
							// registros_cpu,  program_counter,   motivo_desalojo

	serializar_contexto_parcial(paquete->buffer, motivo_desalojo);

	void* a_enviar = malloc(paquete->buffer->size + sizeof(uint8_t) + sizeof(uint32_t));
	int offset = 0;

	memcpy(a_enviar + offset, &(paquete->codigo_operacion), sizeof(uint8_t));
	offset += sizeof(uint8_t);

	memcpy(a_enviar + offset, &(paquete->buffer->size), sizeof(uint32_t));
	offset += sizeof(uint32_t);

	memcpy(a_enviar + offset, paquete->buffer->stream, paquete->buffer->size);

	send(socket_cliente, a_enviar, paquete->buffer->size + sizeof(uint8_t) + sizeof(uint32_t), 0);
	free(a_enviar);
	eliminar_paquete(paquete);
}

void serializar_contexto_parcial(t_buffer* buffer, cod_instruccion motivo_desalojo) {
	int desplazamiento = 0;

	void* stream = malloc(buffer->size);

	memcpy(stream + desplazamiento, &motivo_desalojo, sizeof(uint8_t));
	desplazamiento += sizeof(uint8_t);

	memcpy(stream + desplazamiento, &(contexto->program_counter), sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);

	memcpy(stream + desplazamiento, &(contexto->registros_cpu->AX), 4);
	desplazamiento += 4;
	memcpy(stream + desplazamiento, &(contexto->registros_cpu->BX), 4);
	desplazamiento += 4;
	memcpy(stream + desplazamiento, &(contexto->registros_cpu->CX), 4);
	desplazamiento += 4;
	memcpy(stream + desplazamiento, &(contexto->registros_cpu->DX), 4);
	desplazamiento += 4;
	memcpy(stream + desplazamiento, &(contexto->registros_cpu->EAX), 8);
	desplazamiento += 8;
	memcpy(stream + desplazamiento, &(contexto->registros_cpu->EBX), 8);
	desplazamiento += 8;
	memcpy(stream + desplazamiento, &(contexto->registros_cpu->ECX), 8);
	desplazamiento += 8;
	memcpy(stream + desplazamiento, &(contexto->registros_cpu->EDX), 8);
	desplazamiento += 8;
	memcpy(stream + desplazamiento, &(contexto->registros_cpu->RAX), 16);
	desplazamiento += 16;
	memcpy(stream + desplazamiento, &(contexto->registros_cpu->RBX), 16);
	desplazamiento += 16;
	memcpy(stream + desplazamiento, &(contexto->registros_cpu->RCX), 16);
	desplazamiento += 16;
	memcpy(stream + desplazamiento, &(contexto->registros_cpu->RDX), 16);
	desplazamiento += 16;

	buffer->stream = stream;
}


