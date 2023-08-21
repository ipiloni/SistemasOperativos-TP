#include "estructuras.h"

t_list* tablaProcesosGlobal;
t_list* listaSegmentosGlobal;
t_list* huecosVacios;

void* bloqueMemoria;

void crearSegmentoCero(){
	listaSegmentosGlobal = list_create();
	t_segmento *segmento0 = (t_segmento*)malloc(sizeof(t_segmento));
	segmento0->pid = -1;
	segmento0->idSegmento = 0;
	segmento0->base = 0;
	segmento0->tamanio = config_get_int_value(config, "TAM_SEGMENTO_0");
	list_add(listaSegmentosGlobal,segmento0);

	actualizarHuecosVacios();

}

void inicializarMemoria() {
	int tamanioMemoria = config_get_int_value(config, "TAM_MEMORIA");
	bloqueMemoria = malloc(tamanioMemoria);

	huecosVacios = list_create();
	tablaProcesosGlobal = list_create();
	HuecoVacio* huecoVacio = (HuecoVacio*)malloc(sizeof(HuecoVacio));
	huecoVacio->base = 0;
	huecoVacio->tamanio= tamanioMemoria;
	list_add(huecosVacios, huecoVacio);
}

t_list* crearProceso(uint32_t pid) {

	Proceso* proceso = (Proceso*)malloc(sizeof(Proceso));		//Creo el proceso
	proceso->pid = pid;
	list_add(tablaProcesosGlobal,proceso);


	t_segmento * segmento0 = encontrarSegmentoPorID(0);
	proceso->tablaSegmentos = list_create();
	list_add(proceso->tablaSegmentos,segmento0);

	log_info(logger, "Creacion de Proceso PID: <%d>", pid);

	return proceso->tablaSegmentos;								//Devuelvo la tabla de segmentos del proceso
}

void eliminarProceso(uint32_t pid){
	int esElProcesoPorPID(Proceso *p) {
		  if (p->pid == pid){
		      return true;
		  } else return false;
	}

	int esElSegmentoPorPID(t_segmento *s) {
	     if (s->pid==pid){
	        return true;
	     } else return false;
	 }

	if(encontrarProcesoPorPID(pid)==NULL){
		log_error(logger,"PID: %d no existe y no se puede eliminar ", pid);
		return;
	}
	//Busco en la tabla de segmentos global los segmentos del proceso y los hago pedazos
	list_remove_and_destroy_all_by_condition(listaSegmentosGlobal, (void*)esElSegmentoPorPID,(void*)segmento_destroy );

	//Busco el proceso en mi tabla de procesos y lo destruyo junto con su tabla de segmentos
	list_remove_and_destroy_by_condition(tablaProcesosGlobal,(void*)esElProcesoPorPID, (void*)proceso_destroy);

	actualizarHuecosVacios();
	log_info(logger,"Eliminacion de Proceso PID: <%d> ", pid);
	return;
}

void actualizarHuecosVacios(){

	list_sort(listaSegmentosGlobal, (void*)segmentoMenorBase); // Los ordeno por base
	t_segmento * segmentoActual;
	list_clean_and_destroy_elements(huecosVacios,free);

	int direccionBase = 0;
	for (int i = 0; i < list_size(listaSegmentosGlobal); i++) {
		segmentoActual = list_get(listaSegmentosGlobal, i);
	        if (direccionBase < segmentoActual->base) {
	            // Agregar un hueco vacío antes del segmento actual
	        	HuecoVacio * nuevoHueco = (HuecoVacio*)malloc(sizeof(HuecoVacio));
	            nuevoHueco->base = direccionBase;
	            nuevoHueco->tamanio = segmentoActual->base - direccionBase;
	            list_add(huecosVacios, nuevoHueco);
	        }

	        direccionBase = segmentoActual->base + segmentoActual->tamanio;
	    }
	int tamanioMemoria = config_get_int_value(config, "TAM_MEMORIA");
	if(direccionBase < tamanioMemoria){
		HuecoVacio * nuevoHueco = malloc(sizeof(HuecoVacio));
	    nuevoHueco->base = direccionBase;
	    nuevoHueco->tamanio = tamanioMemoria - direccionBase;
	    list_add(huecosVacios,nuevoHueco);
	}

}

void agregarAHuecosVacios(uint32_t base, uint32_t tamanio) {
	HuecoVacio* nuevoHueco = malloc(sizeof(HuecoVacio));

	nuevoHueco->base = base;
	nuevoHueco->tamanio = tamanio;

	list_add(huecosVacios, nuevoHueco);
	actualizarHuecosVacios();
}

bool segmentoMenorBase(t_segmento* segmentoMenorBase, t_segmento* segmentoMayorBase) {
    return segmentoMenorBase->base <= segmentoMayorBase->base;
}

void leerEspacioUsuario(char* valorALeer,int PID,int direccionFisica, int tamanio, uint8_t idCliente){
	retardoEspacioUsuario();

	char* direccionValor = (char*)bloqueMemoria + direccionFisica;

	// Elimina lo que no sea alfanumerico
	for (int i = 0; i < tamanio; i++) {
		valorALeer[i] = direccionValor[i];
	}
	valorALeer[tamanio] = '\0';

	memcpy(valorALeer, direccionValor, tamanio);

	log_info(logger,"PID: <%d> - Acción: <LEER> - Dirección fisica: <%d> - Tamaño: <%d> - Origen <%s>",PID,direccionFisica,tamanio,(idCliente == CPU) ? "CPU" : "FS");

	return;
}

void escribirEspacioUsuario(char * valorAEscribir,int PID, int direccionFisica, int tamanio, uint8_t idCliente){
	retardoEspacioUsuario();

	char* direccionValor = (char*)bloqueMemoria + direccionFisica;

	memcpy(direccionValor, valorAEscribir , tamanio);

	log_info(logger,"PID: <%d> - Acción: <ESCRIBIR> - Dirección fisica: <%d> - Tamaño: <%d> - Origen <%s>",PID,direccionFisica,tamanio,(idCliente == CPU) ? "CPU" : "FS");

	free(valorAEscribir);
	return;

}

void retardoEspacioUsuario(){
	int retardo = config_get_int_value(config, "RETARDO_MEMORIA")/1000;
	log_debug(logger,"Accediendo al espacio de usuario, espere %d segundos... ", retardo);
	sleep(retardo);
}

int puedoCrearSegmento(int pid, int idSegmento, int tamanio) {
	Proceso* proceso = encontrarProcesoPorPID(pid);
	
	int cant_segmentos = config_get_int_value(config, "CANT_SEGMENTOS");
	//Si la cantidad de segmentos que hay presente es mayor o igual al limite fijo definido por sistema
	if(list_size(proceso->tablaSegmentos) >= cant_segmentos ) return LIMITE_TABLA_SEGMENTOS;

	//Si encuentra un proceso con esa id no lo crea
	if(encontrarSegmentoPorIDYTabla(idSegmento, proceso->tablaSegmentos)!=NULL) return ID_YA_CREADA;

	return hayEspacio(tamanio);
	
}

int crearSegmento(int pid, int idSegmento, int tamanio) {

	t_segmento* nuevoSegmento = malloc(sizeof(t_segmento));
    nuevoSegmento->pid = pid;
    nuevoSegmento->idSegmento = idSegmento;
    nuevoSegmento->tamanio = tamanio;

	char * algoritmo = config_get_string_value(config, "ALGORITMO_ASIGNACION");
	
	if(strcmp(algoritmo, "BEST") == 0) {
		asignarSegmentoBest(nuevoSegmento);
	} else if (strcmp(algoritmo, "FIRST") == 0) {
		asignarSegmentoFirst(nuevoSegmento);
	} else if (strcmp(algoritmo, "WORST") == 0){
		asignarSegmentoWorst(nuevoSegmento);
	}
	
	log_info(logger, "PID: <%d> - Crear Segmento: <%d> - Base: <%d> - Tamaño: <%d> ", pid, idSegmento, nuevoSegmento->base, tamanio);
	return nuevoSegmento->base;
 }

t_list* eliminarSegmento(int pid, int idSegmento) {
	t_segmento* segmento = encontrarSegmentoPorIDYPID(idSegmento,pid);
	Proceso* proceso = encontrarProcesoPorPID(pid);

	if(encontrarSegmentoPorIDYPID(idSegmento,pid) == NULL){
		log_error(logger,"PID: %d intento eliminar el segmento con id %d pero ese segmento no existe ", pid, idSegmento);
		return proceso->tablaSegmentos;
	}

	log_info(logger,"PID: <%d> - Eliminar Segmento: <%d> - Base: <%d>- Tamaño: <%d> ", pid, idSegmento, segmento->base, segmento->tamanio);

	list_remove_element(listaSegmentosGlobal, segmento);
	list_remove_element(proceso->tablaSegmentos, segmento);
	actualizarHuecosVacios();

	free(segmento);

	return proceso->tablaSegmentos;
}

void compactarSegmentos() {
	int retardo = config_get_int_value(config, "RETARDO_COMPACTACION")/1000;
	log_debug(logger,"Compactando segmentos, espere %d segundos...", retardo);
	sleep(retardo);

	HuecoVacio* granHueco = malloc(sizeof(HuecoVacio));

	list_sort(huecosVacios, (void*)huecos_menor_base);
	list_sort(listaSegmentosGlobal,(void*)segmentoMenorBase);

	granHueco->tamanio = 0;

	int baseIdeal = 0;
	log_info(logger,"Segmentos actualizados:");
	log_info(logger,"--------------------------------------------");
	for(int i = 0; i < list_size(listaSegmentosGlobal) - 1; i++) {
		t_segmento* segmento = list_get(listaSegmentosGlobal, i);
		t_segmento* siguienteSegmento = list_get(listaSegmentosGlobal, i+1);
		
		baseIdeal = segmento->base + segmento->tamanio;
		
		if (baseIdeal != siguienteSegmento->base) { //No estan pegados los segmentos (Hay un hueco vacio) <- Hueco sin h? xd
			memcpy(bloqueMemoria+baseIdeal, bloqueMemoria+siguienteSegmento->base, siguienteSegmento->tamanio);
			siguienteSegmento->base = baseIdeal;
			//baseIdeal = siguienteSegmento->base + siguienteSegmento->tamanio;
		}
		
		log_info(logger,"PID: <%d> - Segmento: <%d> - Base: <%d> - Tamaño <%d>", segmento->pid, segmento->idSegmento, segmento->base, segmento->tamanio);
		log_info(logger,"--------------------------------------------");
	}

	t_segmento * ultimoSegmento = list_get(listaSegmentosGlobal, list_size(listaSegmentosGlobal) - 1);
	log_info(logger,"PID: <%d> - Segmento: <%d> - Base: <%d> - Tamaño <%d>", ultimoSegmento->pid, ultimoSegmento->idSegmento, ultimoSegmento->base, ultimoSegmento->tamanio);
	log_info(logger,"--------------------------------------------");

	granHueco->tamanio = config_get_int_value(config, "TAM_MEMORIA") - (ultimoSegmento->base + ultimoSegmento->tamanio);
	granHueco->base = (ultimoSegmento->base + ultimoSegmento->tamanio);

	log_debug(logger, "Nos quedo un hueco vacio base en: %d y un tamaño de: %d ", granHueco->base, granHueco->tamanio);

	list_clean_and_destroy_elements(huecosVacios,free);
	list_add(huecosVacios, granHueco);

}


void imprimirHuecosVacios() {
	log_info(logger,"Huecos vacios actuales en el sistema");
	list_iterate(huecosVacios, (void*) iteradorPrintHuecosVacios);
}

void iteradorPrintHuecosVacios(HuecoVacio * huecoVacio) {
    log_info(logger,"Base %d - Tamaño %d",huecoVacio->base, huecoVacio-> tamanio);
}

void proceso_destroy(Proceso* proceso){
    list_destroy(proceso->tablaSegmentos);
    free(proceso);
}

void segmento_destroy(t_segmento* segmento) {
	free(segmento);
}

t_segmento *encontrarSegmentoPorIDYPID(int idSegmento, int PID) {
	int esElSegmentoPorIDyPID(t_segmento *p) {
	     if (p->idSegmento == idSegmento && p->pid==PID){
	        return true;
	     } else return false;
	 }

     return list_find(listaSegmentosGlobal, (void*) esElSegmentoPorIDyPID);
}

t_segmento *encontrarSegmentoPorID(int idSegmento) {
	int esElSegmentoPorID(t_segmento *s) {
	       if (s->idSegmento == idSegmento){
	          return true;
	       } else return false;
	 }
     return list_find(listaSegmentosGlobal, (void*) esElSegmentoPorID);
}

Proceso *encontrarProcesoPorPID(int PID) {
	int esElProcesoPorPID(Proceso *p) {
			  if (p->pid == PID){
			      return true;
			  } else return false;
	}

     return list_find(tablaProcesosGlobal, (void*) esElProcesoPorPID);
}








