#include "operacionesFS.h"

FILE* archivoDeBloques;

void enviar_mensaje_a_kernel(int socket_kernel, op_code codigo_operacion) {
	op_code respuesta = codigo_operacion;
	send(socket_kernel,&(respuesta),sizeof(uint32_t),0);
}

void crear_archivo(char* nombreArchivo, int socket_kernel){
	t_config* fcb;

	char direccion[256];
	strcpy(direccion, "./fcb/");
	strcat(direccion, nombreArchivo);
 	strcat(direccion, ".fcb");

	FILE* archivoFCBNuevo = fopen(direccion, "w");

	fcb = config_create(direccion);

	config_set_value(fcb, "NOMBRE_ARCHIVO", nombreArchivo);
	config_set_value(fcb, "TAMANIO_ARCHIVO", "0");
	config_set_value(fcb, "PUNTERO_DIRECTO", "NULL");
	config_set_value(fcb, "PUNTERO_INDIRECTO", "NULL");
	config_save(fcb);

	config_destroy(fcb);

	fclose(archivoFCBNuevo);

	log_info(logger, "Crear Archivo: <%s> ", nombreArchivo);

	abrir_archivo(nombreArchivo, socket_kernel);
}

void abrir_archivo(char* nombreArchivo, int socket_kernel) {
    char direccion[256];
    strcpy(direccion, "./fcb/");
    strcat(direccion, nombreArchivo);
	strcat(direccion, ".fcb");

	t_config * fcb = config_create(direccion);

    if(fcb != NULL) {
        log_info(logger, "Abrir Archivo: <%s> ", nombreArchivo);
        enviar_mensaje_a_kernel(socket_kernel, OK);
        config_destroy(fcb);
	} else {
    	log_warning(logger, "El archivo con el nombre %s no existe, espero respuesta de kernel",nombreArchivo);
		enviar_mensaje_a_kernel(socket_kernel, NO_EXISTE_ARCHIVO);
    }

}



char* recibir_nombre_de_kernel(t_buffer* buffer){
	
	uint32_t tamanio;

	memcpy(&tamanio, buffer->stream, sizeof(uint32_t));
	buffer->stream+=sizeof(uint32_t);

	char* valor = malloc(tamanio+1);
	memcpy(valor, buffer->stream, tamanio);

	valor[tamanio] = '\0';
	buffer->stream+=tamanio;
	return valor;
}

void truncar_archivo(char* nombre, int tamanioNuevo){

	if (tamanioNuevo == 0) {
		log_error(logger, "No se puede truncar a cero un archivo");
		return;
	}

	char direccion[256];
	strcpy(direccion, "./fcb/");
	strcat(direccion, nombre);
	strcat(direccion, ".fcb");

	t_config* fcb_archivo;
	fcb_archivo = config_create(direccion);
	double tamanioAnterior = config_get_double_value(fcb_archivo, "TAMANIO_ARCHIVO");
	uint32_t punteroIndirecto = config_get_int_value(fcb_archivo, "PUNTERO_INDIRECTO");
	
	int cant_bloques_anterior = ceiling((double)tamanioAnterior/tamanioBloque);
	int cant_bloques_nueva = ceiling((double)tamanioNuevo/tamanioBloque);

	char * tamanioComoString = itoa(tamanioNuevo);
	config_set_value(fcb_archivo, "TAMANIO_ARCHIVO", tamanioComoString);
	free(tamanioComoString);

	if (tamanioAnterior == 0) { //asignamos puntero directo
		int indiceDeBloque = buscarBloqueLibre();
		char * bloqueLibreComoString = itoa(indiceDeBloque);
		config_set_value(fcb_archivo, "PUNTERO_DIRECTO", bloqueLibreComoString);
		free(bloqueLibreComoString);
		cant_bloques_anterior = 1;
	}
	
	archivoDeBloques = fopen(pathArchBloques, "r+");

	if(cant_bloques_anterior > cant_bloques_nueva) { // liberar bloques
		// Se para en el archivo de bloques en la posicion donde va a empezar a leer el bitmap para liberar los bloques
		int dondeArrancamos = punteroIndirecto*tamanioBloque + (cant_bloques_nueva - 1)*4;

		fseek(archivoDeBloques,dondeArrancamos, SEEK_SET);
		
		int diferenciaDeBloques = cant_bloques_anterior - cant_bloques_nueva;

		int retardo = config_get_int_value(config, "RETARDO_ACCESO_BLOQUE")/1000;
		log_debug(logger,"Accediendo a bloque, espere %d segundos", retardo);
		sleep(retardo);

		log_info(logger, "Acceso Bloque - Archivo: <%s> - Bloque Archivo: <%d> - Bloque File System <%d>", nombre, 2, punteroIndirecto);

		for(int i = 0; i < diferenciaDeBloques; i++) {
			char punteroDirecto[5];
			fgets(punteroDirecto, sizeof(punteroDirecto), archivoDeBloques);
			int bloqueALiberar=atoi(punteroDirecto);
			bitarray_clean_bit(bitmap_bloques, bloqueALiberar);
			log_info(logger, "Acceso Bitmap - Bloque: %d - Nuevo Estado: %d", bloqueALiberar, 0);
		}

		if (cant_bloques_nueva == 1) {
			bitarray_clean_bit(bitmap_bloques, punteroIndirecto);
			log_info(logger, "Acceso Bitmap - Bloque: %d - Nuevo Estado: %d", punteroIndirecto, 0);
			config_set_value(fcb_archivo, "PUNTERO_INDIRECTO", "NULL");
		}

	} else if (cant_bloques_anterior < cant_bloques_nueva) { // asignar bloques

		if(cant_bloques_anterior == 1) { // crear un punteroIndirecto
			int indiceDeBloque = buscarBloqueLibre();
			char * bloqueLibreComoString = itoa(indiceDeBloque);
			config_set_value(fcb_archivo, "PUNTERO_INDIRECTO", bloqueLibreComoString);
			free(bloqueLibreComoString);
		}

		uint32_t punteroIndirecto = config_get_int_value(fcb_archivo, "PUNTERO_INDIRECTO");

		int dondeArrancamos = punteroIndirecto*tamanioBloque + (cant_bloques_anterior - 1)*4;

		fseek(archivoDeBloques, dondeArrancamos, SEEK_SET);

		int diferenciaDeBloques = cant_bloques_nueva - cant_bloques_anterior;

		int retardo = config_get_int_value(config, "RETARDO_ACCESO_BLOQUE")/1000;
		log_debug(logger,"Accediendo a bloque, espere %d segundos", retardo);
		sleep(retardo);
		//es el bloque 2 porque el punteroindirecto del archivo siempre es el segundo bloque del archivo
		log_info(logger, "Acceso Bloque - Archivo: <%s> - Bloque Archivo: <%d> - Bloque File System <%d>", nombre, 2, punteroIndirecto);

		for(int i = 0; i < diferenciaDeBloques; i++) {
			int indiceDeBloque = buscarBloqueLibre();
			char* puntero = malloc(sizeof(char) * 5);
    		snprintf(puntero, 5, "%04d", indiceDeBloque); // pone 5 porque tiene en cuenta el \0
    		fputs(puntero, archivoDeBloques);
    		free(puntero);

		}
	}
	
	fclose(archivoDeBloques);
	
	config_save(fcb_archivo);

	msync(bitarray,tamanioBitArray,MS_SYNC);

	config_destroy(fcb_archivo);

	log_info(logger,"Truncar Archivo: <%s> - Tamaño <%d>", nombre, tamanioNuevo);


}

int buscarBloqueLibre(){ //buscarlo y ocuparlo
	int indice = 0;
	while(bitarray_test_bit(bitmap_bloques, indice) == 1){
		indice++;
	}

	bitarray_set_bit(bitmap_bloques, indice);
	log_info(logger, "Acceso a Bitmap - Bloque: <%d> - Estado: <%d>", indice, 1);
	return indice;
}

void acceder_archivo(char* nombreArchivo, int punteroArchivo, int dir_fisica ,int tamanioTotalAcceso, int pid ,op_code modo, int socket_memoria){

	char direccion[256];
	strcpy(direccion, "./fcb/");
	strcat(direccion, nombreArchivo);
	strcat(direccion, ".fcb");

	t_config* fcb_archivo;
	fcb_archivo = config_create(direccion);

	int tamanioArchivo = config_get_int_value(fcb_archivo, "TAMANIO_ARCHIVO");
	uint32_t punteroIndirecto = config_get_int_value(fcb_archivo, "PUNTERO_INDIRECTO");
	uint32_t punteroDirecto = config_get_int_value(fcb_archivo, "PUNTERO_DIRECTO");

	int posicionFinal = punteroArchivo + tamanioTotalAcceso;

	if(posicionFinal > tamanioArchivo){
		log_error(logger,"El archivo no tiene esa capacidad para leer /escribir esa cantidad de bytes");
		return;
	}

	char* todoLoQueEscriboOLeo;

	if(modo == ESCRIBIR_ARCHIVO){
		todoLoQueEscriboOLeo = solicitar_a_memoria(dir_fisica, socket_memoria, tamanioTotalAcceso, pid);
	} else if(modo == LEER_ARCHIVO){
		todoLoQueEscriboOLeo = malloc(tamanioTotalAcceso +1);
	}

	char* string = todoLoQueEscriboOLeo;

	archivoDeBloques = fopen(pathArchBloques, "r+");

	//Accedo a bloques
	if(posicionFinal < tamanioBloque){//estoy en el primer bloque y necesito un cachito del mismo

		accederADirecto(punteroArchivo, tamanioTotalAcceso, punteroDirecto, &string, modo, nombreArchivo,0);

	} else if(posicionFinal > tamanioBloque && punteroArchivo<tamanioBloque) { //estoy en el primer bloque y necesito mas indirectos

		int tamanioRestanteEnBloqueDirecto = tamanioBloque - punteroArchivo;
		accederADirecto(punteroArchivo,tamanioRestanteEnBloqueDirecto, punteroDirecto, &string, modo,nombreArchivo,0); //accedo a lo que queda del bloque directo
		int tamanioRestanteParaIndirectos = tamanioTotalAcceso - tamanioRestanteEnBloqueDirecto;

		int retardo = config_get_int_value(config, "RETARDO_ACCESO_BLOQUE")/1000;
		log_debug(logger,"Accediendo a bloque, espere %d segundos", retardo);
		sleep(retardo);

		log_info(logger, "Acceso Bloque - Archivo: <%s> - Bloque Archivo: <%d> - Bloque File System <%d>", nombreArchivo, 1, punteroIndirecto);

		accederAIndirectosRestantes(0,tamanioRestanteParaIndirectos,punteroIndirecto, &string,modo,nombreArchivo);//arranco desde el bloque indirecto simple

	} else if(punteroArchivo>=tamanioBloque) { // estoy en un indirecto

			int retardo = config_get_int_value(config, "RETARDO_ACCESO_BLOQUE")/1000;
			log_debug(logger,"Accediendo a bloque, espere %d segundos", retardo);
			sleep(retardo);

			log_info(logger, "Acceso Bloque - Archivo: <%s> - Bloque Archivo: <%d> - Bloque File System <%d>", nombreArchivo, 1, punteroIndirecto);
			// Usamos el resto del bloque indirecto
			int bloqueIndirectoActual = ceiling((double)punteroArchivo /tamanioBloque)-1;
			fseek(archivoDeBloques,punteroIndirecto * tamanioBloque + (bloqueIndirectoActual)*4, SEEK_SET);
			char punteroDirectoActual[5] = "";
			fread(&punteroDirectoActual, 1, 4 , archivoDeBloques);
			punteroDirectoActual[4] = '\0';
			int punteroDirecto = atoi(punteroDirectoActual);

			int offsetDentroDeBloque = punteroArchivo % tamanioBloque;

			int tamanioRestanteDentroDeIndirecto = tamanioBloque-offsetDentroDeBloque;

			if((offsetDentroDeBloque + tamanioTotalAcceso)>tamanioRestanteDentroDeIndirecto){ //estoy en un indirecto y necesitas mas indirectos
				accederADirecto(offsetDentroDeBloque,tamanioRestanteDentroDeIndirecto, punteroDirecto , &string, modo,nombreArchivo,bloqueIndirectoActual+1+1); //bloque directo+bloque de punteros
				int tamanioRestanteParaIndirectos = tamanioTotalAcceso-tamanioRestanteDentroDeIndirecto;
				accederAIndirectosRestantes((bloqueIndirectoActual+1),tamanioRestanteParaIndirectos,punteroIndirecto, &string,modo,nombreArchivo);
			} else {//estas en un indirecto y solo necesitas un cachito de ese indirecto
				accederADirecto(offsetDentroDeBloque,tamanioTotalAcceso, punteroDirecto , &string, modo,nombreArchivo,bloqueIndirectoActual+1+1); // bloque directo + bloque de punteros
			}

	}
	todoLoQueEscriboOLeo[tamanioTotalAcceso] = '\0';

	log_debug(logger, "%s archivo: %s", modo==LEER_ARCHIVO ? "Leí esto del" : "Escribí esto en el", todoLoQueEscriboOLeo);

	if(modo == LEER_ARCHIVO){
		enviar_a_memoria(socket_memoria, dir_fisica, todoLoQueEscriboOLeo, pid, tamanioTotalAcceso);
	}

	fclose(archivoDeBloques);
	free(todoLoQueEscriboOLeo);
	log_info(logger, "%s Archivo: <%s> - Puntero: <%d> - Memoria: <%d> - Tamaño: <%d>",(modo == ESCRIBIR_ARCHIVO) ? "Escribir" : "Leer", nombreArchivo, punteroArchivo, dir_fisica, tamanioTotalAcceso);
	config_destroy(fcb_archivo);
}

void accederADirecto(int offsetDentroDeBloque , int tamanioAcceso, int punteroDirecto,char** string, op_code modo,char *nombreArchivo,int bloqueDelArchivo){

	fseek(archivoDeBloques,punteroDirecto * tamanioBloque + offsetDentroDeBloque, SEEK_SET);

	if(modo == ESCRIBIR_ARCHIVO){
		fwrite(*string, 1 ,tamanioAcceso, archivoDeBloques);

	} else if(modo == LEER_ARCHIVO){
		fread(*string, 1, tamanioAcceso , archivoDeBloques);
	}

	int retardo = config_get_int_value(config, "RETARDO_ACCESO_BLOQUE")/1000;
	log_debug(logger,"Accediendo a bloque, espere %d segundos", retardo);
	sleep(retardo);

	log_info(logger, "Acceso Bloque - Archivo: <%s> - Bloque Archivo: <%d> - Bloque File System <%d>", nombreArchivo, bloqueDelArchivo, punteroDirecto);

	*string += tamanioAcceso; //Avanzo el puntero a la cadena de caracteres
}

void accederAIndirectosRestantes(int bloqueIndirectoInicial,int tamanioRestanteParaIndirectos, int punteroIndirecto,char** string, op_code modo,char* nombreArchivo){

	int bloquesNecesitados = ceiling((double)tamanioRestanteParaIndirectos/tamanioBloque);

	int dondeArrancamosEnArchivo = punteroIndirecto*tamanioBloque + bloqueIndirectoInicial*4;

	int cuantoLeemosEnBloqueFinal = tamanioRestanteParaIndirectos % tamanioBloque;

	fseek(archivoDeBloques, dondeArrancamosEnArchivo, SEEK_SET); // pararse al inicio del indirecto

	for(int i = 0; i < bloquesNecesitados; i++) {

		fseek(archivoDeBloques, dondeArrancamosEnArchivo + (4 * i), SEEK_SET);

		char punteroDirecto[5] = ""; // quizas sea de 4
		fread(&punteroDirecto, 1, 4 , archivoDeBloques); // lee el primer puntero a bloque directo

		if(tamanioRestanteParaIndirectos>tamanioBloque){ // si no alcanza con un bloque
			int direccionBloque = atoi(punteroDirecto);
			accederADirecto(0, tamanioBloque, direccionBloque, string, modo, nombreArchivo,bloqueIndirectoInicial+i+1+1); // hay que tener en cuenta el bloque directo y, además, el bloque de punteros y marcos solo tuvo en cuenta el bloque... no se, alguno de los dos tuvo en cuenta
			tamanioRestanteParaIndirectos -= tamanioBloque;
		} else {

			int direccionBloqueDirecto = atoi(punteroDirecto);
			accederADirecto(0, cuantoLeemosEnBloqueFinal, direccionBloqueDirecto,string,modo,nombreArchivo,bloqueIndirectoInicial+i+1+1); // " diem
		}

	}
}
