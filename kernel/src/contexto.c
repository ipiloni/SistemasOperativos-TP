#include "contexto.h"

void enviar_contexto_a_cpu(int socket_cliente) {

	t_contexto* contexto = obtener_contexto_de_pcb(pcb_en_ejecucion);

	t_paquete* paquete = crear_paquete();
	paquete->codigo_operacion = CONTEXTO_DE_EJECUCION;
	
	serializar_contexto_de_ejecucion(contexto, paquete->buffer); // añade al buffer size las instrucciones

	int offset = 0;
	void* a_enviar = malloc(paquete->buffer->size + sizeof(uint8_t) + sizeof(uint32_t));

	memcpy(a_enviar + offset, &(paquete->codigo_operacion), sizeof(uint8_t));
	offset += sizeof(uint8_t);

	memcpy(a_enviar + offset, &(paquete->buffer->size), sizeof(uint32_t));
	offset += sizeof(uint32_t);

	memcpy(a_enviar + offset, paquete->buffer->stream, paquete->buffer->size);

	send(socket_cliente, a_enviar, paquete->buffer->size + sizeof(uint8_t) + sizeof(uint32_t), 0);

	free(a_enviar);
	free(contexto);
	eliminar_paquete(paquete);
}

t_contexto* obtener_contexto_de_pcb(t_pcb* pcb) {
	t_contexto* contexto = malloc(sizeof(t_contexto));

	contexto->cant_instrucciones = pcb->cantidad_instrucciones;
	contexto->instrucciones = pcb->instrucciones;
	contexto->pid = pcb->pid;
	contexto->program_counter = pcb->program_counter;
	contexto->registros_cpu = pcb->registros_cpu;
	contexto->cant_segmentos = list_size(pcb->tabla_de_segmentos);
	contexto->tabla_de_segmentos = pcb->tabla_de_segmentos;

	return contexto;
}

void serializar_contexto_de_ejecucion(t_contexto* contexto, t_buffer* buffer) {
	int desplazamiento = 0;

	buffer->size = (4*4 + 4*8 + 4*16) + 4*(sizeof(uint32_t)) + (4*sizeof(uint32_t)) * contexto->cant_segmentos;
	// (tamanio registros) + pid + cant instrucciones + program counter + cant segmentos + (tamanio segmentos) * cant segmentos

	t_instruccion* instruccion;
	for (int i=0; i < contexto->cant_instrucciones; i++) {
		instruccion = list_get(contexto->instrucciones, i);
		buffer->size += sizeof(uint32_t) * 4 + instruccion->nombre_length + instruccion->parametro1_length + instruccion->parametro2_length + instruccion->parametro3_length;
	}

	buffer->stream = malloc(buffer->size);
	void* stream = buffer->stream;

	memcpy(stream + desplazamiento, &(contexto->pid), sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);

	memcpy(stream + desplazamiento, &(contexto->cant_instrucciones), sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);

	for (int i=0; i < contexto->cant_instrucciones; i++) {
		instruccion = list_get(contexto->instrucciones, i);

		memcpy(stream + desplazamiento, &(instruccion->nombre_length), sizeof(uint32_t));
		desplazamiento += sizeof(uint32_t);
		memcpy(stream + desplazamiento, instruccion->nombre, instruccion->nombre_length);
		desplazamiento += instruccion->nombre_length;

		memcpy(stream + desplazamiento, &(instruccion->parametro1_length), sizeof(uint32_t));
		desplazamiento += sizeof(uint32_t);
		memcpy(stream + desplazamiento, instruccion->parametro1, instruccion->parametro1_length);
		desplazamiento += instruccion->parametro1_length;

		memcpy(stream + desplazamiento, &(instruccion->parametro2_length), sizeof(uint32_t));
		desplazamiento += sizeof(uint32_t);
		memcpy(stream + desplazamiento, instruccion->parametro2, instruccion->parametro2_length);
		desplazamiento += instruccion->parametro2_length;

		memcpy(stream + desplazamiento, &(instruccion->parametro3_length), sizeof(uint32_t));
		desplazamiento += sizeof(uint32_t);
		memcpy(stream + desplazamiento, instruccion->parametro3, instruccion->parametro3_length);
		desplazamiento += instruccion->parametro3_length;
	}

	memcpy(stream + desplazamiento, &(contexto->program_counter), sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);

	memcpy(stream + desplazamiento, contexto->registros_cpu->AX, 4);
	desplazamiento += 4;
	memcpy(stream + desplazamiento, contexto->registros_cpu->BX, 4);
	desplazamiento += 4;
	memcpy(stream + desplazamiento, contexto->registros_cpu->CX, 4);
	desplazamiento += 4;
	memcpy(stream + desplazamiento, contexto->registros_cpu->DX, 4);
	desplazamiento += 4;
	memcpy(stream + desplazamiento, contexto->registros_cpu->EAX, 8);
	desplazamiento += 8;
	memcpy(stream + desplazamiento, contexto->registros_cpu->EBX, 8);
	desplazamiento += 8;
	memcpy(stream + desplazamiento, contexto->registros_cpu->ECX, 8);
	desplazamiento += 8;
	memcpy(stream + desplazamiento, contexto->registros_cpu->EDX, 8);
	desplazamiento += 8;
	memcpy(stream + desplazamiento, contexto->registros_cpu->RAX, 16);
	desplazamiento += 16;
	memcpy(stream + desplazamiento, contexto->registros_cpu->RBX, 16);
	desplazamiento += 16;
	memcpy(stream + desplazamiento, contexto->registros_cpu->RCX, 16);
	desplazamiento += 16;
	memcpy(stream + desplazamiento, contexto->registros_cpu->RDX, 16);
	desplazamiento += 16;

	memcpy(stream + desplazamiento, &(contexto->cant_segmentos), sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);

	for (int i = 0; i < contexto->cant_segmentos; i++) {
        t_segmento* segmento = list_get(contexto->tabla_de_segmentos, i);

		memcpy(stream + desplazamiento, &(segmento->pid), sizeof(uint32_t));
		desplazamiento += sizeof(uint32_t);
        memcpy(stream + desplazamiento, &(segmento->idSegmento), sizeof(uint32_t));
		desplazamiento += sizeof(uint32_t);
		memcpy(stream + desplazamiento, &(segmento->base), sizeof(uint32_t));
		desplazamiento += sizeof(uint32_t);
		memcpy(stream + desplazamiento, &(segmento->tamanio), sizeof(uint32_t));
		desplazamiento += sizeof(uint32_t);
    }

	buffer->stream = stream;
}
