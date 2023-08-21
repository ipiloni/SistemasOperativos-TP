#ifndef MAIN_H_
#define MAIN_H_

#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <commons/log.h>
#include <commons/string.h>
#include <commons/config.h>
#include <commons/collections/list.h>

#include <utils/utils.h>
#include "conexiones.h"
#include "estructuras.h"
#include "operaciones.h"

extern t_config* config;
extern t_log* logger;
extern int handshakesCorrectos;
extern sem_t handshakeTerminado;
extern sem_t semaforoProcesarConexion[];

t_log* iniciar_logger(void);
void inicializarSemaforos(void);
void testIfEstructurasFunciona(void);

#endif /* SERVER_H_ */
