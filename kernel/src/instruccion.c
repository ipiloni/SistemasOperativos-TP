#include "instruccion.h"

void mostrar_instruccion(void* instruccion) {
	t_instruccion* instruccion_auxiliar = (t_instruccion*) instruccion;

	printf("Nombre de instruccion: %s\t Parametro 1: %s\t Parametro 2: %s\t Parametro3: %s\n", instruccion_auxiliar->nombre, instruccion_auxiliar->parametro1, instruccion_auxiliar->parametro2, instruccion_auxiliar->parametro3);
}

t_instruccion* crear_instruccion(char* nombre, char* parametro1, char* parametro2, char* parametro3){
    t_instruccion* ret = malloc(sizeof(t_instruccion));

    ret->nombre_length = strlen(nombre)+1;
    ret->nombre = strdup(nombre);
    ret->parametro1_length = strlen(parametro1)+1;
    ret->parametro1 = strdup(parametro1);
    ret->parametro2_length = strlen(parametro2)+1;
    ret->parametro2 = strdup(parametro2);
    ret->parametro3_length = strlen(parametro3)+1;
    ret->parametro3 = strdup(parametro3);
    return ret;
}

void cargar_instrucciones(FILE* f_instrucciones,t_list* lista_instrucciones){
	char linea[TAMANIO_LINEA];
	char* nombre;
	char* parametro1;
	char* parametro2;
	char* parametro3;

	while(fgets(linea, TAMANIO_LINEA, f_instrucciones) != NULL){
		size_t indice_nulo = strcspn(linea, "\n");
		if (indice_nulo != strlen(linea)) {
		         linea[indice_nulo] = '\0';
		    }

			char** array = string_split(linea, " "); //memory leak
			//SET AX 1
			//["SET", "AX", "1\n", NULL];

			nombre = array[0];
			if(nombre == NULL){
				string_array_destroy(array);
				exit(4);
			}
			parametro1 = array[1];
			if(parametro1 == NULL){
				list_add(lista_instrucciones, crear_instruccion(nombre, "vacio", "vacio", "vacio"));
				string_array_destroy(array);
				continue;
			}

			parametro2 = array[2];
			if(parametro2 == NULL){
				list_add(lista_instrucciones, crear_instruccion(nombre, parametro1, "vacio", "vacio"));
				string_array_destroy(array);
				continue;
			}

			parametro3 = array[3];
			if(parametro3 == NULL){
				list_add(lista_instrucciones, crear_instruccion(nombre, parametro1, parametro2, "vacio"));
				string_array_destroy(array);
				continue;
			}

			list_add(lista_instrucciones, crear_instruccion(nombre, parametro1, parametro2, parametro3));
			string_array_destroy(array);
		}
}

t_list* deserializar_lista_instrucciones(t_buffer* buffer_instrucciones) {
	t_list* instrucciones = list_create();
	int cantidad_instrucciones = 0;

	void* stream = buffer_instrucciones->stream; // aca se muere todo

	memcpy(&cantidad_instrucciones, stream, sizeof(uint32_t));
	stream += sizeof(uint32_t);

	printf("cantidad de instrucciones recibidas: %d", cantidad_instrucciones); // esto no aparece

	for(int i = 0; i<cantidad_instrucciones; i++) {
		t_instruccion* instruccion = malloc(sizeof(t_instruccion));

		memcpy(&(instruccion->nombre_length), stream, sizeof(uint32_t));
		stream += sizeof(uint32_t);
		memcpy(&(instruccion->nombre), stream, instruccion->nombre_length);

		stream += instruccion->nombre_length;

		memcpy(&(instruccion->parametro1_length), stream, sizeof(uint32_t));
		stream += sizeof(uint32_t);
		memcpy(&(instruccion->parametro1), stream, instruccion->parametro1_length);

		stream += instruccion->parametro1_length;

		memcpy(&(instruccion->parametro2_length), stream, sizeof(uint32_t));
		stream += sizeof(uint32_t);
		memcpy(&(instruccion->parametro2), stream, instruccion->parametro2_length);

		stream += instruccion->parametro2_length;

		memcpy(&(instruccion->parametro3_length), stream, sizeof(uint32_t));
		stream += sizeof(uint32_t);
		memcpy(&(instruccion->parametro3), stream, instruccion->parametro3_length);

		list_add(instrucciones, instruccion);
	}
	return instrucciones;
}
