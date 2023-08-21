#ifndef CPU_H_
#define CPU_H_

#include <utils/utils.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <stdio.h>
#include <stdlib.h>
#include <commons/log.h>
#include <commons/string.h>
#include <commons/config.h>
#include <readline/readline.h>
#include <string.h>
#include <math.h>
#include <sys/time.h>

#include "instruccion.h"
#include "comunicacion.h"

extern t_log* logger;
extern t_config* config;
extern t_contexto* contexto;
extern t_registros_cpu* registros;
extern int tamanio_max_segmento;
extern int se_necesita_cambio_de_contexto;

#define SEM_CPU_MEMORIA 1234

t_log* iniciar_logger(void);
t_config* iniciar_config(void);
void inicializar_contexto(t_contexto*);
void terminar_programa(int, t_log*, t_config*);
uint32_t obtener_dir_fisica(char*, uint32_t bytes);
uint32_t obtener_num_segmento(uint32_t);
uint32_t obtener_desplazamiento_segmento(uint32_t);

#endif /* CPU_H_ */
