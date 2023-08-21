#include "instruccion.h"

void copiar_cadena(char* registro, char* valor, int longitud) {
    int i;
    for (i = 0; i < longitud; i++) {
    	registro[i] = valor[i];
    }
}

void instruccion_set(char* registro, char* valor){
    if(strcmp(registro, "AX") == 0) {
        memcpy(contexto->registros_cpu->AX, valor, 4);
        log_debug(logger, "Actualizacion de Registro AX: %.*s", 4, contexto->registros_cpu->AX);
    }
    else if(strcmp(registro, "BX") == 0) {
        memcpy(contexto->registros_cpu->BX, valor, 4);
        log_debug(logger, "Actualizacion de Registro BX: %.*s", 4, contexto->registros_cpu->BX);
    }
    else if(strcmp(registro, "CX") == 0) {
        memcpy(contexto->registros_cpu->CX, valor, 4);
        log_debug(logger, "Actualizacion de Registro CX: %.*s", 4,  contexto->registros_cpu->CX);
    }
    else if(strcmp(registro, "DX") == 0) {
        memcpy(contexto->registros_cpu->DX, valor, 4);
        log_debug(logger, "Actualizacion de Registro DX: %.*s", 4, contexto->registros_cpu->DX);
    }
    else if(strcmp(registro, "EAX") == 0) {
        memcpy(contexto->registros_cpu->EAX, valor, 8);
        log_debug(logger, "Actualizacion de Registro EAX: %.*s", 8, contexto->registros_cpu->EAX);
    }
    else if(strcmp(registro, "EBX") == 0) {
        memcpy(contexto->registros_cpu->EBX, valor, 8);
        log_debug(logger, "Actualizacion de Registro EBX: %.*s", 8, contexto->registros_cpu->EBX);
    }
    else if(strcmp(registro, "ECX") == 0) {
        memcpy(contexto->registros_cpu->ECX, valor, 8);
        log_debug(logger, "Actualizacion de Registro ECX: %.*s", 8, contexto->registros_cpu->ECX);
    }
    else if(strcmp(registro, "EDX") == 0) {
        memcpy(contexto->registros_cpu->EDX, valor, 8);
        log_debug(logger, "Actualizacion de Registro EDX: %.*s", 8, contexto->registros_cpu->EDX);
    }
    else if(strcmp(registro, "RAX") == 0) {
        memcpy(contexto->registros_cpu->RAX, valor, 16);
        log_debug(logger, "Actualizacion de Registro RAX: %.*s", 16, contexto->registros_cpu->RAX);
    }
    else if(strcmp(registro, "RBX") == 0) {
        memcpy(contexto->registros_cpu->RBX, valor, 16);
        log_debug(logger, "Actualizacion de Registro RBX: %.*s", 16, contexto->registros_cpu->RBX);
    }
    else if(strcmp(registro, "RCX") == 0) {
        memcpy(contexto->registros_cpu->RCX, valor, 16);
        log_debug(logger, "Actualizacion de Registro RCX: %.*s", 16, contexto->registros_cpu->RCX);
    }
    else if(strcmp(registro, "RDX") == 0) {
        memcpy(contexto->registros_cpu->RDX, valor, 16);
        log_debug(logger, "Actualizacion de Registro RDX: %.*s", 16, contexto->registros_cpu->RDX);
    }
}

t_instruccion* fetch_instruccion(t_contexto* contexto){
	t_instruccion* instruccion;

	instruccion = list_get(contexto->instrucciones, contexto->program_counter);

	return instruccion;
}

void decode_instruccion(t_instruccion* instruccion, int socket, int conexion_memoria) {
	char* nombre = instruccion->nombre;
	char* param1 = instruccion->parametro1;
	char* param2 = instruccion->parametro2;
	char* param3 = instruccion->parametro3;

	cod_instruccion codigo = obtener_enum_de_instruccion(nombre);

	/* Esto es solo para que no muestre "vacio" */
	if(strcmp(param3, "vacio") == 0) {
		param3 = "";
		if(strcmp(param2, "vacio") == 0) {
			param2 = "";
			if(strcmp(param1, "vacio") == 0) {
				param1 = "";
			}
		}
	}

	log_info(logger, "PID: %d - PC: %d - Ejecutando: %s - %s %s %s ", contexto->pid, contexto->program_counter, nombre, param1, param2, param3);

	switch (codigo){

		case SET:

		int retardo_instruccion;
		retardo_instruccion = config_get_int_value(config, "RETARDO_INSTRUCCION");
		sleep(retardo_instruccion/1000);

		instruccion_set(param1, param2);
		contexto->program_counter++;

		break;

		case MOV_IN:

		uint32_t bytes = obtener_cant_bytes(param1);
		int id_segmento = obtener_id_segmento(param2);
		uint32_t direccion = obtener_dir_fisica(param2, bytes);

		if(direccion < 0) {
			devolver_contexto(socket, SEG_FAULT);
			se_necesita_cambio_de_contexto = 1;
		} else {

			char* valor = solicitar_a_memoria(direccion, conexion_memoria, bytes, contexto->pid);
			log_info(logger, "PID: %d - Acción: LEER - Segmento: %d - Dirección Física: %d - Valor: %.*s", contexto->pid, id_segmento, direccion, bytes, valor);
			instruccion_set(param1, valor);
			free(valor);
			contexto->program_counter++;
		}

		break;

		case MOV_OUT:

		bytes = obtener_cant_bytes(param2);
		id_segmento = obtener_id_segmento(param1);
		direccion = obtener_dir_fisica(param1, bytes);
		if(direccion == -1) {

			devolver_contexto(socket, SEG_FAULT);
			se_necesita_cambio_de_contexto = 1;

		} else {

		char* valor = obtener_valor_de_registro(param2);
		log_info(logger, "PID: %d - Acción: ESCRIBIR - Segmento: %d - Dirección Física: %d - Valor: %.*s ", contexto->pid, id_segmento, direccion, bytes, valor);
		op_code resultado = enviar_a_memoria(conexion_memoria, direccion, valor, contexto->pid, bytes); // hace falta enviar PID?

			if (resultado == OK) {
				log_info(logger, "La escritura se ha realizado de forma correcta.");
				contexto->program_counter++;
			} else log_error(logger, "La escritura no fue realizada correctamente.");

		}

		break;


		case F_READ:

		bytes = atoi(param3);
		direccion = obtener_dir_fisica(param2, bytes);

		if(direccion == -1) {
			devolver_contexto(socket, SEG_FAULT);
			se_necesita_cambio_de_contexto = 1;
		}
		devolver_contexto(socket, F_READ);
		enviar_direccion_fisica(socket, direccion);
		se_necesita_cambio_de_contexto = 1;

		break;

		case F_WRITE:

		bytes = atoi(param3);
		direccion = obtener_dir_fisica(param2, bytes);

		if(direccion == -1) {
			devolver_contexto(socket, SEG_FAULT);
			se_necesita_cambio_de_contexto = 1;
		}
		devolver_contexto(socket, F_WRITE);
		enviar_direccion_fisica(socket, direccion);
		se_necesita_cambio_de_contexto = 1;

		break;

		default:
		//io wait signal fopen fclose ftruncate create segment delete segment yield exit hacen lo mismo
		devolver_contexto(socket,codigo);
		se_necesita_cambio_de_contexto = 1;
	}

}


char* obtener_valor_de_registro(char* registro) {
	if(strcmp(registro, "AX") == 0) {
		return contexto->registros_cpu->AX;
	}
	else if(strcmp(registro, "BX") == 0) {
		return contexto->registros_cpu->BX;
	}
	else if(strcmp(registro, "CX") == 0) {
		return contexto->registros_cpu->CX;
	}
	else if(strcmp(registro, "DX") == 0) {
		return contexto->registros_cpu->DX;
	}
	else if(strcmp(registro, "EAX") == 0) {
		return contexto->registros_cpu->EAX;
	}
	else if(strcmp(registro, "EBX") == 0) {
		return contexto->registros_cpu->EBX;
	}
	else if(strcmp(registro, "ECX") == 0) {
		return contexto->registros_cpu->ECX;
	}
	else if(strcmp(registro, "EDX") == 0) {
		return contexto->registros_cpu->EDX;
	}
	else if(strcmp(registro, "RAX") == 0) {
		return contexto->registros_cpu->RAX;
	}
	else if(strcmp(registro, "RBX") == 0) {
		return contexto->registros_cpu->RBX;
	}
	else if(strcmp(registro, "RCX") == 0) {
		return contexto->registros_cpu->RCX;
	}
	else if(strcmp(registro, "RDX") == 0) {
		return contexto->registros_cpu->RDX;
	} else return "";
}


uint32_t obtener_cant_bytes(char* registro) {
	if(strcmp(registro, "AX") == 0) {
		return 4;
	}
	else if(strcmp(registro, "BX") == 0) {
		return 4;
	}
	else if(strcmp(registro, "CX") == 0) {
		return 4;
	}
	else if(strcmp(registro, "DX") == 0) {
		return 4;
	}
	else if(strcmp(registro, "EAX") == 0) {
		return 8;
	}
	else if(strcmp(registro, "EBX") == 0) {
		return 8;
	}
	else if(strcmp(registro, "ECX") == 0) {
		return 8;
	}
	else if(strcmp(registro, "EDX") == 0) {
		return 8;
	}
	else if(strcmp(registro, "RAX") == 0) {
		return 16;
	}
	else if(strcmp(registro, "RBX") == 0) {
		return 16;
	}
	else if(strcmp(registro, "RCX") == 0) {
		return 16;
	}
	else if(strcmp(registro, "RDX") == 0) {
		return 16;
	} else return 0;
}

int obtener_id_segmento(char* direccion_logica){

	uint32_t direccion = atoi(direccion_logica);
	return obtener_num_segmento(direccion);

}

