#include "comunicacion_filesystem.h"

void* comunicacion_FS(void* arg) {
	t_conexion_info conexion_info_filesystem;
	conexion_info_filesystem = *((t_conexion_info*) arg);
	free(arg);
	pthread_detach(pthread_self());
	tabla_global_de_archivos_abiertos = list_create();
	int socket_filesystem = crear_conexion_handshake(conexion_info_filesystem.ip, conexion_info_filesystem.puerto, 2);

	if(socket_filesystem == 1){
		close(socket_filesystem);
		puts("socket error"); //perror pero errno seria modificado concurrentemente con lo cual necesitaria mutex para todo y mucho lio
		return NULL;
	}

	while (1){

		sem_wait(&operacion_fs);
		t_pcb* pcb_operacion_fs = pcb_en_ejecucion; // mantener una referencia a la pcb_en_ejecucion antes de replanificar
		sem_post(&puede_replanificar);

		t_instruccion* instruccion_pendiente = pcb_operacion_fs->instruccion_pendiente;

		char* nombre_archivo_solicitado = instruccion_pendiente->parametro1;

		cod_instruccion codigo = obtener_enum_de_instruccion(instruccion_pendiente->nombre);

		uint32_t respuesta;

		switch (codigo) {
		case F_OPEN:

			if(preguntar_a_fs_si_existe_archivo_y_abrirlo(instruccion_pendiente,socket_filesystem) == NO_EXISTE_ARCHIVO) {
				solicitar_a_fs(CREAR_ARCHIVO, instruccion_pendiente, 0, 0, socket_filesystem, 0);
				recv(socket_filesystem, &(respuesta), sizeof(uint32_t), MSG_WAITALL);

			}

			t_archivo_global* archivo_abierto_global = inicializar_archivo_global(nombre_archivo_solicitado);

			sem_wait(&mutex_archivos);
			list_add(tabla_global_de_archivos_abiertos, archivo_abierto_global);
			sem_post(&mutex_archivos);

			sem_post(&termina_operacion_fs);
		break;

		case F_TRUNCATE:
			solicitar_a_fs(RECORTAR_ARCHIVO, instruccion_pendiente, 0, 0, socket_filesystem, 0);

			pasar_a_blocked(pcb_operacion_fs, nombre_archivo_solicitado);

			recv(socket_filesystem, &(respuesta), sizeof(uint32_t), MSG_WAITALL);

			if (respuesta == OK) { //aca tira depends on unitialized variables si se corta la comunicacion
				log_info(logger, "PID: <%d> - Truncar archivo <%s> - Tamaño: <%s>", pcb_operacion_fs->pid, nombre_archivo_solicitado, instruccion_pendiente->parametro2);
				pcb_operacion_fs->program_counter++;
				pasar_a_ready(pcb_operacion_fs);
			}
		break;

		case F_READ:
			//aca replanifica
			t_archivo_proceso* archivo = buscarArchivoPorNombreLocal(nombre_archivo_solicitado, pcb_operacion_fs->tabla_archivos_abiertos);

			sem_wait(&mutex_operacion_fs_memoria);

			solicitar_a_fs(LEER_ARCHIVO, instruccion_pendiente, pcb_operacion_fs->pid, archivo->puntero, socket_filesystem, direccion_fisica);


			pasar_a_blocked(pcb_operacion_fs, nombre_archivo_solicitado);

			//Este hilo debe quedarse esperando la respuesta de fs y bloquear al proceso (replanificar)

			recv(socket_filesystem, &(respuesta), sizeof(uint32_t), MSG_WAITALL);

			sem_post(&mutex_operacion_fs_memoria);//para que no se compacten segmentos al mismo tiempo que esto
		    if (respuesta == OK) {
		    	log_info(logger, "PID: <%d> - Leer Archivo <%s> - Puntero <%d> - Dirección Memoria <%s> - Tamaño: <%s>", pcb_operacion_fs->pid, nombre_archivo_solicitado, archivo->puntero, instruccion_pendiente->parametro2, instruccion_pendiente->parametro3);
		    	pcb_operacion_fs->program_counter++;
		    	pasar_a_ready(pcb_operacion_fs);
			}
		    break;

		case F_WRITE:
			//archivo declarado en fread
			archivo = buscarArchivoPorNombreLocal(nombre_archivo_solicitado, pcb_operacion_fs->tabla_archivos_abiertos);

			sem_wait(&mutex_operacion_fs_memoria);
			solicitar_a_fs(ESCRIBIR_ARCHIVO, instruccion_pendiente, pcb_operacion_fs->pid, archivo->puntero, socket_filesystem,direccion_fisica);

			pasar_a_blocked(pcb_operacion_fs, nombre_archivo_solicitado);
			//Este hilo debe quedarse esperando la respuesta de fs y bloquear al proceso (replanificar)

			recv(socket_filesystem, &(respuesta), sizeof(uint32_t), MSG_WAITALL);

			sem_post(&mutex_operacion_fs_memoria);
		    if (respuesta == OK) {
		    	pcb_operacion_fs->program_counter++;
		    	log_info(logger, "PID: <%d> - Escribir Archivo: <%s> - Puntero <%d> - Dirección Memoria <%s> - Tamaño <%s>", pcb_operacion_fs->pid, nombre_archivo_solicitado, archivo->puntero, instruccion_pendiente->parametro2, instruccion_pendiente->parametro3);
		    	pasar_a_ready(pcb_operacion_fs);
			}
		    break;
		default:
			log_error(logger, "Operacion desconocida por File System.");
			break;
		}
	}
	return NULL;
}


t_archivo_global *buscarArchivoPorNombreGlobal(char * nombre_archivo) {

	int comparador_nombre_archivo_global(t_archivo_global * archivo) {
		return string_equals_ignore_case(archivo->nombre_archivo, nombre_archivo);
	}

     return list_find(tabla_global_de_archivos_abiertos, (void*) comparador_nombre_archivo_global);
}

t_archivo_proceso *buscarArchivoPorNombreLocal(char * nombre_archivo, t_list * listaLocal) {
	int comparador_nombre_archivo_local(t_archivo_proceso * archivo) {
		return string_equals_ignore_case(archivo->nombre_archivo, nombre_archivo);
	}

     return list_find(listaLocal, (void*) comparador_nombre_archivo_local);
}

t_archivo_proceso* inicializar_archivo_local(char* nombre){
	t_archivo_proceso* archivo = malloc(sizeof(t_archivo_proceso));
	archivo->nombre_archivo = string_duplicate(nombre);
	archivo->puntero = 0;
	return archivo;
}

t_archivo_global* inicializar_archivo_global(char* nombre){
	t_archivo_global* archivo = malloc(sizeof(t_archivo_global));
	archivo->nombre_archivo = string_duplicate(nombre);
	archivo->cola_esperando = queue_create();
	archivo->bit_de_uso = 0;
	return archivo;
}

bool estaEnUso(t_archivo_global* archivo) {
	return archivo->bit_de_uso;
}

bool estaEnLaTablaGlobal(t_archivo_global* archivo) {
	return archivo != NULL;
}

void eliminar_archivo_local(t_archivo_proceso* archivo){
	free(archivo->nombre_archivo);
	free(archivo);
}

void eliminar_archivo_global(t_archivo_global* archivo){
	free(archivo->nombre_archivo);
	queue_destroy(archivo->cola_esperando);
	free(archivo);
}

void imprimir_archivos_abiertos() {
	log_debug(logger,"Los archivos de esta lista son: ");
	list_iterate(tabla_global_de_archivos_abiertos, (void*) iterador_print_archivo_global);
}

void iterador_print_archivo_global(t_archivo_global* archivo_global) {
    log_debug(logger,"Archivo: <%s> Bit de uso en <%d>", archivo_global->nombre_archivo, archivo_global->bit_de_uso);
}
