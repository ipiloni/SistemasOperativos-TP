#ifndef KERNEL_SRC_MAIN_H_
#define KERNEL_SRC_MAIN_H_

#include <utils/utils.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <commons/log.h>
#include <commons/config.h>
#include <commons/collections/queue.h>
#include <time.h>
#include "conexion.h"
#include "planificador.h"
#include "contexto.h"
#include "instruccion.h"
#include "comunicacion_memoria.h"
#include "comunicacion_cpu.h"
#include "comunicacion_filesystem.h"


extern t_queue* cola_new;
extern t_queue* cola_ready;

extern t_list* lista_pcbs;
extern t_list* tabla_global_de_archivos_abiertos;
extern t_list* lista_recursos;

extern t_config* kernel_config;

extern sem_t proceso_nuevo;
extern sem_t proceso_running;
extern sem_t grado_multiprogramacion_disponible;
extern sem_t mutex_cola_new;
extern sem_t tope_ejecucion;
extern sem_t sem_replanificar;
extern sem_t cantidad_procesos;
extern sem_t mutex_lista_pcbs;
extern sem_t mutex_cola_ready;
extern sem_t inicializar_estructuras;
extern sem_t estructuras_inicializadas;
extern sem_t sem_corto_plazo;
extern sem_t mutex_operacion_fs_memoria;
extern sem_t mutex_archivos;
extern sem_t operacion_fs;
extern sem_t termina_operacion_fs;
extern sem_t termina_operacion_memoria;
extern sem_t puede_replanificar;
extern sem_t operacion_memoria;
extern sem_t eliminar_estructuras;
extern sem_t estructuras_eliminadas;
extern sem_t sem_replanificar;

extern pthread_mutex_t mutex_proceso_finalizado; //TODO reemplazar los sem_t mutex por pthread_mutex_t
extern pthread_cond_t finaliza_proceso;
extern t_info_finalizacion proceso_finalizado;

extern int sin_memoria;
extern uint32_t direccion_fisica;



char* lista_pids(void);
int ultimo_pid(void);
void* comunicacion_consola(void* arg);
void* bloquear_por_tiempo (void* arg);
void terminar_programa(int, t_config*, t_log*, t_list*, t_paquete*);
char* traductor_motivo_desalojo(t_motivo_desalojo motivo_desalojo);
void iniciar_recursos(char** recursos,char** instancias_recursos);


#endif /* KERNEL_SRC_MAIN_H_ */

