#include "algoritmos.h"


int hayEspacio(int tamanio){
    if(hayEspacioContiguo(tamanio) == OK){
        return OK;
    } 
    return hayEspacioPeroNoContiguo(tamanio);
}

int hayEspacioContiguo(int tamanio) {
    
    int tieneEspacioSuficiente(HuecoVacio* huecoVacio) {
        return huecoVacio->tamanio >= tamanio;
    }
    
    HuecoVacio* huecoVacio = list_find(huecosVacios, (void*) tieneEspacioSuficiente);

    if(huecoVacio != NULL) return OK;
    else return OUT_OF_MEMORY;
}

int hayEspacioPeroNoContiguo(int tamanio) {
    int suma = 0;
    for(int i = 0; i < list_size(huecosVacios) ; i++) {
        HuecoVacio* huecoVacio = list_get(huecosVacios, i);
        suma += huecoVacio->tamanio;
    }

    if(suma >= tamanio) return NECESITA_COMPACTACION;
    else return OUT_OF_MEMORY;
}

void asignarSegmentoFirst(t_segmento* segmento) {
    // chequear en el primer elemento de la lista de huecosVacios si entra el segmento
    int tieneEspacioSuficiente(HuecoVacio* huecoVacio) {
        return huecoVacio->tamanio >= segmento->tamanio;
    }
    
    list_sort(huecosVacios, (void*)huecos_menor_base);
    HuecoVacio* huecoVacio = list_remove_by_condition(huecosVacios, (void*) tieneEspacioSuficiente);
    
    if (huecoVacio != NULL) {
    segmento->base = huecoVacio->base;
    list_add(listaSegmentosGlobal, segmento);

    Proceso * proceso = encontrarProcesoPorPID(segmento->pid);
    list_add(proceso->tablaSegmentos, segmento);

    huecoVacio->tamanio -= segmento->tamanio;
    if(huecoVacio->tamanio > 0) {
        huecoVacio->base += segmento->tamanio;
        list_add(huecosVacios, huecoVacio);
    } else {
    	free(huecoVacio);
    }
    }

}

bool huecos_menor_base(HuecoVacio *huecoMenorBase, HuecoVacio *huecoGrandeBase) {
	   return huecoMenorBase->base <= huecoGrandeBase->base;
}


void asignarSegmentoWorst(t_segmento* segmento) {
    // le asignamos el hueco vacio de mayor tamaño

    HuecoVacio* huecoVacio = (HuecoVacio*) list_get_maximum(huecosVacios, (void*) huecoMayorTamanio);
    list_remove_element(huecosVacios, huecoVacio);

    if (huecoVacio != NULL && huecoVacio->tamanio >= segmento->tamanio) {
        segmento->base = huecoVacio->base;
        list_add(listaSegmentosGlobal, segmento);

        Proceso * proceso = encontrarProcesoPorPID(segmento->pid);
        list_add(proceso->tablaSegmentos, segmento);

        huecoVacio->tamanio -= segmento->tamanio;
        if(huecoVacio->tamanio > 0) {
            huecoVacio->base += segmento->tamanio;
            list_add(huecosVacios, huecoVacio);
        } else {
        	free(huecoVacio);
        }
    }
    return;
}

void* huecoMayorTamanio(HuecoVacio* hueco1, HuecoVacio* hueco2) {
    return hueco1->tamanio >= hueco2->tamanio ? hueco1 : hueco2;
}


void asignarSegmentoBest(t_segmento* segmento) {
    // le asignamos el hueco vacio de mejor tamaño
    HuecoVacio* huecoVacio;

    int hueco_index = 0;
    int tamanioBest = 10000000;
    for(int i=0; i < list_size(huecosVacios); i++) {
        HuecoVacio* huecoAux = list_get(huecosVacios, i);
        if(huecoAux->tamanio >= segmento->tamanio && huecoAux->tamanio<tamanioBest ) {
            	tamanioBest = huecoAux->tamanio;
                huecoVacio = huecoAux;
                hueco_index = i;
        }
    }
    
    if(huecoVacio != NULL) {
        list_remove(huecosVacios, hueco_index);
        segmento->base = huecoVacio->base;

        Proceso * proceso = encontrarProcesoPorPID(segmento->pid);
        list_add(proceso->tablaSegmentos, segmento);

        list_add(listaSegmentosGlobal, segmento);

        huecoVacio->tamanio -= segmento->tamanio;
        if(huecoVacio->tamanio > 0) {
            huecoVacio->base += segmento->tamanio;
            list_add(huecosVacios, huecoVacio);
        } else {
        	free(huecoVacio);
        }

    }
    return;
}




/* OPCION BEST en caso de emergencia tire de la palanca!
 * void* huecoQueMejorEncaja(HuecoVacio* hueco1, HuecoVacio* hueco2) {
    return tamanioDespuesDeAsignar(hueco1) <= tamanioDespuesDeAsignar(hueco2) ? hueco1 : hueco2;
}

int tamanioDespuesDeAsignar(HuecoVacio* hueco) {
    int tamanioRestante;

    if(hueco->tamanio >= segmento->tamanio)
     tamanioRestante = hueco->tamanio - segmento->tamanio;
    else tamanioRestante = 10000000

    return tamanioRestante;
}
*/
