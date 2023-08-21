
#ifndef SETUPFILESYSTEM_H_
#define SETUPFILESYSTEM_H_
#include <commons/bitarray.h>

#include "main.h"
#include <dirent.h>

void levantarSuperbloque(t_config* superBloque,int *tamanioBloques, int *cantidadBloques);
void extraerDeConfig(char **path_bitmap, char **path_FCB, char **path_archivoBloques, char **path_superbloque, char **ip, char **puerto_memoria, char **puerto_escucha);
t_log* iniciar_logger(void);
void levantar_bitmap(t_conexion_info_memoria* info_memoria);
#endif /* SETUPFILESYSTEM_H_ */
