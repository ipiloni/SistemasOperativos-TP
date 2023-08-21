#include "setupFilesystem.h"

void levantarSuperbloque(t_config* superBloque,int *tamanioBloques, int *cantidadBloques)
{
	*tamanioBloques = config_get_int_value(superBloque, "BLOCK_SIZE");
	*cantidadBloques = config_get_int_value(superBloque, "BLOCK_COUNT");
}

void extraerDeConfig(char **path_bitmap, char **path_FCB, char **path_archivoBloques, char **path_superbloque, char **ip, char **puerto_memoria, char **puerto_escucha)
{
	*path_bitmap = config_get_string_value(config, "PATH_BITMAP");
	*path_FCB = config_get_string_value(config, "PATH_FCB");
	*path_archivoBloques = config_get_string_value(config, "PATH_BLOQUES");
	*path_superbloque = config_get_string_value(config, "PATH_SUPERBLOQUE");
	*ip = config_get_string_value(config, "IP_MEMORIA");
	*puerto_memoria = config_get_string_value(config, "PUERTO_MEMORIA");
	*puerto_escucha = config_get_string_value(config, "PUERTO_ESCUCHA");
}

t_log* iniciar_logger(void)
{
    t_log* nuevo_logger;
    nuevo_logger= log_create("logFilesystem.log", "Servidor", 1, LOG_LEVEL_DEBUG);
    return nuevo_logger;
}

void levantar_bitmap(t_conexion_info_memoria* info_memoria){

	FILE* fd = fopen(info_memoria->path_bitmap, "rb+");
	if (fd == NULL) {
		log_error(logger,"Error al abrir el archivo de bitmap");
		return;
	}
	//Mapeamos el bitarray a "memoria"
	bitarray = mmap(NULL, tamanioBitArray,PROT_READ | PROT_WRITE, MAP_SHARED, fileno(fd), 0);

	if (bitarray == MAP_FAILED) {
		log_error(logger,"Error al mapear el bitarray");
		fclose(fd);
		return;
	}

	bitmap_bloques = bitarray_create_with_mode(bitarray, tamanioBitArray, MSB_FIRST); //LSB O MSB, no estoy seguro cual iria, commons dice que vayamos por esta en ese caso
	//Setear si el archivo no esta inicializado (que el archivo este vacio no quiere decir que no tenga cosas, esta lleno de NULLs)
	if(config_get_int_value(config, "FORMATEAR")) {
		for(int i= 0; i < tamanioBitArray*8;i++){
			bitarray_clean_bit(bitmap_bloques,i);
		}
		//Sincroniza el bitarray al archivo
		msync(bitarray,tamanioBitArray,MS_SYNC);
		FILE* archivoDeBloques = fopen(pathArchBloques, "w");
		//fseek(archivoDeBloques,0, SEEK_SET);
		for (int i = 0; i < tamanioBloque*tamanioBitArray; i++) {
		        fputc('0', archivoDeBloques); // Write '0' character to the file
		}
		fclose(archivoDeBloques);

		//borra los fcb de los archivos
		DIR* directorioFCB = opendir("fcb");
		struct dirent *next_file;
		char filepath[260];
		while ((next_file = readdir(directorioFCB)) != NULL ) {
			if (strcmp(next_file->d_name, ".") == 0 || strcmp(next_file->d_name, "..") == 0)
			continue;
			// build the path for each file in the folder
			sprintf(filepath, "%s/%s", "fcb", next_file->d_name);
			remove(filepath);
		}
		closedir(directorioFCB);
	}
	log_debug(logger,"Bitarray de %lu bits",bitarray_get_max_bit(bitmap_bloques));
	fclose(fd);
}
