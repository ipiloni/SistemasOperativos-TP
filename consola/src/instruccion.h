#ifndef CONSOLA_CONSOLAH
#define CONSOLA_CONSOLAH

#define TAMANIO_LINEA 30
#define TAMANIO_INSTRUCCION 20
#define CARACTER_NO_ENCONTRADO 21

#include <utils/utils.h>
#include <stdio.h>
#include <stdlib.h>
#include <commons/collections/list.h>
#include <commons/string.h>
#include <commons/config.h>
#include <string.h>
#include <stdint.h>

#define INSTRUCCION_INVALIDA 10

/*
Parsear lineas del pseudocódigo en instrucciones y generar un listado con las mismas.

Conectar al Kernel.

Enviar listado de instrucciones, junto con tamaño de los segmentos.

Recibir confirmación de recepción.

Esperar nuevos mensajes del Kernel. (Imprimir por pantalla o solicitar un valor por teclado)

Informar al Kernel una vez finalizadas para que desbloquee el proceso
*/

int parsear_instrucciones(char linea, t_instruccion);
void mostrar_instruccion(void* instrucccion);
t_instruccion* crear_instruccion(char* nombre, char* parametro1, char* parametro2, char* parametro3);
void cargar_instrucciones(FILE* t_instrucciones,t_list* lista_instrucciones);

int crear_conexion(char* ip, char* puerto);
void enviar_mensaje(char* mensaje, int socket_cliente);
void liberar_conexion(int socket_cliente);
t_list* deserializar_lista_instrucciones(t_paquete*);

#endif /* CONSOLA_CONSOLAH */
