#include <commons/collections/queue.h>
#include <commons/temporal.h>

#include "main.h"

extern int pid_actual;

t_pcb* crear_pcb(t_list* lista_instrucciones);
void pcb_destroy(t_pcb* pcb);
bool pcb_cargada_en_lista(t_pcb* pcb);

void* planificador_largo_plazo(void* arg);
void* planificador_corto_plazo(void* arg);
void cambiar_estado_proceso(t_pcb*, estado_proceso);
char* estado_a_string(estado_proceso);
void pasar_a_ready(t_pcb* pcb);
void pasar_a_blocked(t_pcb* pcb, char* motivo_bloqueo);
t_motivo_desalojo op_code_a_motivo_desalojo(op_code codigo_operacion);
void* bloquear_por_tiempo(void*);
void*  pcbs_highest_ratio_response (t_pcb* pcb1, t_pcb* pcb2);

void eliminar_registros(t_registros_cpu* registros);
void terminar_proceso_en_ejecucion(t_motivo_desalojo motivo_desalojo, int socket_memoria);

void calcular_ratio_response (t_pcb* pcb);
void calcular_estimado_ejecucion(t_pcb* pcb);
void replanificar(int* flag_replanificar);

//se puede abstraer mejor. agregar_a(t_pcb*, t_list*);
//quitar_de(t_pcb*, t_list*);
//pasar_a(t_pcb*, estado_proceso);
