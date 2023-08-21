#ifndef SRC_INSTRUCCION_H_
#define SRC_INSTRUCCION_H_

#include "main.h"

void copiar_cadena(char*, char*, int);
void instruccion_set(char*, char*);
t_instruccion* fetch_instruccion(t_contexto*); //recibir instruccion
void decode_instruccion(t_instruccion*, int, int); //interpretar
int segmentation_fault(int, int);
char* obtener_valor_de_registro(char*);
int obtener_id_segmento(char*);
uint32_t obtener_cant_bytes(char*);

#endif /* SRC_INSTRUCCION_H_ */
