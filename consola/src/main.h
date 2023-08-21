#ifndef CONSOLA_SRC_MAIN_H_
#define CONSOLA_SRC_MAIN_H_

#include <utils/utils.h>
#include <stdio.h>
#include <stdlib.h>
#include <commons/string.h>
#include <commons/config.h>
#include <commons/log.h>
#include "instruccion.h"

#define SOCKET_ERROR 5
#define SEND_ERROR 6
#define HANDSHAKE_FALLIDO 9
#define PROCESO_FINALIZADO 10

extern t_log* logger;

t_log* iniciar_logger(void);
t_config* iniciar_config(char*);
void terminar_programa(int conexion, t_log*, t_config*, FILE*, t_list*, t_paquete*);

#endif /* CONSOLA_SRC_MAIN_H_ */
