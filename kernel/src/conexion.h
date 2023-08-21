#include "main.h"

#define HANDSHAKE_FALLIDO 9
#define PROCESO_FINALIZADO 10

extern t_log* logger;
extern t_list* lista_global_de_pcbs;
void* recibir_cliente(void*); // op_code con hilos
int responder_handshake(int);
void atender_cliente(t_paquete*, int socket_cliente);
void enviar_pid_a_memoria(int socket_memoria, uint32_t pid, op_code cod_op);
void solicitar_creacion_segmento(int socket_memoria, uint32_t pid, uint32_t idSegmento, uint32_t tamanio);
void solicitar_eliminacion_segmento(int socket_memoria, uint32_t pid, uint32_t idSegmento);
void solicitar_a_fs(op_code codigo, t_instruccion* instruccion, uint32_t pid, uint32_t puntero, int socket_fs, uint32_t direccion_fisica);
void solicitar_compactacion(int socket_memoria);
uint32_t preguntar_a_fs_si_existe_archivo_y_abrirlo(t_instruccion* instruccion, int socket_fs);
t_pcb* obtener_pcb_por_id(int pid);
void recibir_tablas_de_segmentos(int socket_memoria);
