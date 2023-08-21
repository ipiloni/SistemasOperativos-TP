#include "instruccion.h"

#include <stdio.h>
#include <stdlib.h>


void mostrar_instruccion(void* instruccion) {
	t_instruccion* instruccion_auxiliar = (t_instruccion*) instruccion;

	printf("Nombre de instruccion: %s\t Parametro 1: %s\t Parametro 2: %s\t Parametro3: %s\n", instruccion_auxiliar->nombre, instruccion_auxiliar->parametro1, instruccion_auxiliar->parametro2, instruccion_auxiliar->parametro3);
}

t_instruccion* crear_instruccion(char* nombre, char* parametro1, char* parametro2, char* parametro3){
    t_instruccion* ret = malloc(sizeof(t_instruccion));

    ret->nombre_length = strlen(nombre)+1;
    ret->nombre = malloc(ret->nombre_length);
    strcpy(ret->nombre, nombre);

    ret->parametro1_length = strlen(parametro1)+1;
    ret->parametro1 = malloc(ret->parametro1_length);
    strcpy(ret->parametro1, parametro1);

    ret->parametro2_length = strlen(parametro2)+1;
    ret->parametro2 = malloc(ret->parametro2_length);
    strcpy(ret->parametro2, parametro2);

    ret->parametro3_length = strlen(parametro3)+1;
    ret->parametro3 = malloc(ret->parametro3_length);
    strcpy(ret->parametro3, parametro3);

    return ret;
}

void cargar_instrucciones(FILE* f_instrucciones,t_list* lista_instrucciones){
	char linea[TAMANIO_LINEA];

	while(fgets(linea, TAMANIO_LINEA, f_instrucciones) != NULL){
		size_t indice_nulo = strcspn(linea, "\n");
		if (indice_nulo != strlen(linea)) {
		         linea[indice_nulo] = '\0';
		    }

		//char* instruccion[4] = {"vacio", "vacio", "vacio", "vacio"};

			char** array = string_split(linea, " "); //memory leak
			//SET AX 1
			//["SET", "AX", "1\n", NULL];
			//["EXIT", NULL];
			//["YIELD\n", NULL];
			//["I/O", "10\n", NULL];

			switch(string_array_size(array)) {
				case 0:
					exit(INSTRUCCION_INVALIDA);
				case 1:
					list_add(lista_instrucciones, crear_instruccion(array[0], "vacio", "vacio", "vacio"));
					break;
				case 2:
					list_add(lista_instrucciones, crear_instruccion(array[0], array[1], "vacio", "vacio"));
					break;
				case 3:
					list_add(lista_instrucciones, crear_instruccion(array[0], array[1], array[2], "vacio"));
					break;
				case 4:
					list_add(lista_instrucciones, crear_instruccion(array[0], array[1], array[2], array[3]));
					break;
			}
			string_array_destroy(array);
		}
}

t_list* deserializar_lista_instrucciones(t_paquete* paquete_instrucciones) {

	int cantidad_instrucciones = 0;

	t_list* instrucciones = list_create();

	//void* stream = buffer_instrucciones->stream; // aca se muere todo

	memcpy(&cantidad_instrucciones, paquete_instrucciones->buffer->stream, sizeof(uint32_t));
	paquete_instrucciones->buffer->stream += sizeof(uint32_t);

	printf("CANTIDAD AYUDA: %d", cantidad_instrucciones);

	puts("lei la cantidad de instrucciones");

	for(int i = 0; i<cantidad_instrucciones; i++) {
		t_instruccion* instruccion = deserializar_instruccion(paquete_instrucciones->buffer);
		paquete_instrucciones->buffer->stream += sizeof(instruccion->nombre_length + instruccion->parametro1_length + instruccion->parametro2_length + instruccion->parametro3_length);
		list_add(instrucciones, instruccion);
	}
	return instrucciones;
}
