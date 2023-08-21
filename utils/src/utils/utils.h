#ifndef UTILS_H_
#define UTILS_H_

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <stddef.h>
#include <assert.h>
#include <commons/log.h>
#include <commons/temporal.h>
#include <commons/collections/list.h>
#include <commons/collections/queue.h>

typedef enum
{
	ERROR =-1,
	OUT_OF_MEMORY,
	ID_YA_CREADA,
	LIMITE_TABLA_SEGMENTOS,
	MENSAJE,
	PAQUETE,
	PAQUETE_INSTRUCCIONES,
	INSTRUCCION,
	CONTEXTO_DE_EJECUCION,
	TABLA_SEGMENTOS,
	CREAR_PROCESO,
	ELIMINAR_PROCESO,
	CREAR_SEGMENTO,
	ELIMINAR_SEGMENTO,
	COMPACTAR,
	ESCRIBIR_ESPACIO_USUARIO,
	LEER_ESPACIO_USUARIO,
	OK,
	NECESITA_COMPACTACION,
	ABRIR_ARCHIVO,
	CREAR_ARCHIVO,
	CERRAR_ARCHIVO,
	POSICIONAR_EN_ARCHIVO,
	LEER_ARCHIVO,
	RECORTAR_ARCHIVO,
	ESCRIBIR_ARCHIVO,
	NO_EXISTE_ARCHIVO,
	ARCHIVO_ABIERTO
}op_code;

typedef enum{
	NEW,
	READY,
	BLOCKED,
	RUNNING,
	ESTADO_EXIT
} estado_proceso;

typedef enum
{
	MEMORIA,
	CPU,
	KERNEL,
	FILESYSTEM,
	CONSOLA
}handshake;

typedef enum
{
	SET,
	MOV_IN,
	MOV_OUT,
	IO, //cpu no hace nada
	F_OPEN,//cpu no hace nada
	F_CLOSE,//cpu no hace nada
	F_SEEK,//cpu no hace nada
	F_READ,
	F_TRUNCATE,//cpu no hace nada
	F_WRITE,
	WAIT, //cpu no hace nada
	SIGNAL, //cpu no hace nada
	CREATE_SEGMENT, //cpu no hace nada
	DELETE_SEGMENT, //cpu no hace nada
	YIELD, //cpu no hace nada
	EXIT, //cpu no hace nada
	SEG_FAULT
}cod_instruccion;

typedef struct {
	char AX[4];
	char BX[4];
	char CX[4];
	char DX[4];
	char EAX[8];
	char EBX[8];
	char ECX[8];
	char EDX[8];
	char RAX[16];
	char RBX[16];
	char RCX[16];
	char RDX[16];
} t_registros_cpu;

typedef struct
{
	uint32_t nombre_length;
    char* nombre;

    uint32_t parametro1_length;
    char* parametro1;

    uint32_t parametro2_length;
    char* parametro2;

    uint32_t parametro3_length;
    char* parametro3;
} t_instruccion;

typedef struct {
	uint32_t pid;
	estado_proceso estado;
	t_instruccion* instruccion_pendiente;

	//Contexto
	uint32_t cantidad_instrucciones;
	t_list* instrucciones;
	uint32_t program_counter;
	t_registros_cpu* registros_cpu;
	t_list* tabla_de_segmentos;
	t_list* tabla_archivos_abiertos;

	//HRRN
	double estimacion_proxima_rafaga;
	time_t tiempo_llegada_a_ready;
	double tiempo_en_ready;
	time_t tiempo_llegada_running;
	double real_ejecucion_anterior;
	double ratio_response;
}t_pcb;

typedef enum {
	MOTIVO_DESCONOCIDO = 1,
	SEGMENTATION_FAULT,
	FINALIZA_PROCESO,
	SIN_MEMORIA,
	RECURSO_INVALIDO,
	SEGMENTO_YA_CREADO,
	SIN_ESPACIO_EN_TABLA
}t_motivo_desalojo;

typedef struct {
	int pid;
	t_motivo_desalojo motivo_desalojo;
} t_info_finalizacion;

typedef struct {
	uint32_t pid;
	uint32_t idSegmento;
	uint32_t base;
	uint32_t tamanio;
} t_segmento;

typedef struct {
	t_pcb* pcb_bloqueado;
	unsigned int tiempo_bloqueado;
}t_bloqueador_proceso;

typedef struct {
	char* nombre_recurso;
	int instancias;
	t_queue* procesos_bloqueados;
}t_recurso;

typedef struct {

	uint32_t pid;
	uint32_t cant_instrucciones;
	t_list* instrucciones;
	uint32_t program_counter;
	t_registros_cpu* registros_cpu;
	uint32_t cant_segmentos;
	t_list* tabla_de_segmentos;
} t_contexto;

typedef struct
{
	int size;
	void* stream;
} t_buffer;

typedef struct
{
	op_code codigo_operacion;
	t_buffer* buffer;
} t_paquete;

typedef struct {
    char ip[16];   // Dirección IP (máximo 15 caracteres)
    char puerto[6];    // Puerto
    int socket_memoria;
} t_conexion_info;

extern t_log* logger;


//Deserializaciones
void inicializar_registros_cpu(t_registros_cpu*);
t_instruccion* deserializar_instruccion(t_buffer*);
void instruccion_destroy(t_instruccion*);
t_list* recibir_tabla_segmentos(int socket_memoria);
t_segmento* deserializar_segmento(t_buffer* buffer);
void* serializar_lista_segmentos(t_list* listaSegmentos, int* serializedSize);
t_list* deserializar_lista_segmentos(void* serializedData, int serializedSize);

//Cliente
int crear_conexion(char* ip, char* puerto);
int crear_conexion_handshake(char* ip, char* puerto, int handshake);
void enviar_mensaje(char* mensaje, int socket_cliente);
t_paquete* crear_paquete(void);
void agregar_a_paquete(t_paquete* paquete, void* valor, int tamanio);
void enviar_paquete(t_paquete* paquete, int socket_cliente);
void liberar_conexion(int socket_cliente);
void eliminar_paquete(t_paquete* paquete);
char* solicitar_a_memoria(uint32_t, int, uint32_t, uint32_t);
char* recibir_resultado_memoria(int, uint32_t);
op_code enviar_a_memoria(int, uint32_t, char*, uint32_t, uint32_t);
t_segmento* encontrarSegmentoPorIDYTabla(int idSegmento,t_list * tablaSegmentos);
char* obtener_cod_op(op_code codigo);
char* obtener_modulo(handshake modulo);
char* obtener_motivo(cod_instruccion motivo);
cod_instruccion obtener_enum_de_instruccion(char * string_instruccion);
//Servidor
void* recibir_buffer(int*, int);
int iniciar_servidor(char*);
int esperar_cliente(int);
void recibir_mensaje(int);
int recibir_operacion(int);
t_paquete * recibir_paquete(t_paquete *  paquete, int socket);

//Impresiones
void imprimirSegmentos(t_list* lista);
void iteradorPrintTablaSegmentos(t_segmento* segmento);
char * itoa(int);
int ceiling(double x);
#endif /* UTILS_H_ */
