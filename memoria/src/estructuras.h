#ifndef ESTRUCTURAS_H
#define ESTRUCTURAS_H

#include "main.h"

extern t_list* tablaProcesosGlobal;

extern t_list* listaSegmentosGlobal;
extern t_list* huecosVacios;

typedef struct {
	uint32_t pid;
	t_list* tablaSegmentos;
} Proceso;

typedef struct {
	uint32_t base;
	uint32_t tamanio;
} HuecoVacio;

#include "algoritmos.h" //NO MOVER ESTA LINEA DE ACA

//Inicializar
void inicializarMemoria();
void crearSegmentoCero();

//Printear
void imprimirSegmentos(t_list* lista);
void iteradorPrintTablaSegmentos(t_segmento* segmento);
void imprimirHuecosVacios();
void iteradorPrintHuecosVacios(HuecoVacio* huecoVacio);

//Destructores
void segmento_destroy(t_segmento* segmento);
void proceso_destroy(Proceso* proceso);

//Operaciones
t_list* crearProceso(uint32_t pid);
void eliminarProceso(uint32_t pid);
int puedoCrearSegmento(int pid, int idSegmento, int tamanio);
int crearSegmento(int pid, int idSegmento, int tamanio);
t_list* eliminarSegmento(int pid, int idSegmento);
void compactarSegmentos();
void leerEspacioUsuario(char * valorALeer,int PID, int direccionFisica, int tamanio, uint8_t idCliente);
void escribirEspacioUsuario(char* valorAEscribir, int PID,int direccionFisica,  int tamanio, uint8_t idCliente);

//Utilidades
void retardoEspacioUsuario();
bool segmentoMenorBase(t_segmento* segmento1, t_segmento* segmento2);
void actualizarListaProcesos();
void actualizarHuecosVacios();
void agregarAHuecosVacios(uint32_t base, uint32_t tamanio);
t_segmento *encontrarSegmentoPorID(int idSegmento);
t_segmento *encontrarSegmentoPorIDYPID(int idSegmento, int PID);
Proceso *encontrarProcesoPorPID(int PID);

#endif /* ESTRUCTURAS_H */
