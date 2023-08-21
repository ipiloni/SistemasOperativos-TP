#ifndef ALGORTIMOS_H_
#define ALGORTIMOS_H_

#include "estructuras.h"

int tieneEspacioSuficiente(HuecoVacio*);
void* huecoMayorTamanio(HuecoVacio* hueco1, HuecoVacio* hueco2);
bool huecos_menor_base(HuecoVacio *huecoMenorBase, HuecoVacio *huecoGrandeBase);
int hayEspacio(int tamanio);
int hayEspacioContiguo(int tamanio);
int hayEspacioPeroNoContiguo(int tamanio);
void asignarSegmentoBest(t_segmento* segmento);
void asignarSegmentoWorst(t_segmento* segmento);
void asignarSegmentoFirst(t_segmento* segmento);
#endif /* ALGORTIMOS_H_ */
