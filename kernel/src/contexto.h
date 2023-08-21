#ifndef SRC_CONTEXTO_H_
#define SRC_CONTEXTO_H_

#include "main.h"

extern t_pcb* pcb_en_ejecucion;

void enviar_contexto_a_cpu(int socket_cliente);
t_contexto* obtener_contexto_de_pcb(t_pcb*);
void serializar_contexto_de_ejecucion(t_contexto*, t_buffer*);

#endif /* SRC_CONTEXTO_H_ */
